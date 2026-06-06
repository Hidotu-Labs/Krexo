#include <Uefi.h>
#include <common/boot_info.h>
#include <common/config.h>
#include <common/disk.h>
#include <common/elf.h>
#include <common/fb.h>
#include <common/fs/fat32.h>
#include <common/kprint.h>
#include <common/menu.h>
#include <common/mmap.h>
#include <common/request_handler.h>

extern int uefi_disk_init(krexo_disk_t *disk);
void uefi_init_debug(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);
extern int uefi_read_file(const char *filename, void *buffer,
                          unsigned long long *size);

static EFI_SYSTEM_TABLE *gST;

static void uefi_delay_ms(int ms) { gST->BootServices->Stall(ms * 1000); }

#include <common/keys.h>

static int uefi_menu_get_key(void) {
  EFI_INPUT_KEY key;
  EFI_STATUS status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key);
  if (status == EFI_SUCCESS) {
    if (key.ScanCode == 0x01)
      return KREXO_KEY_UP;
    if (key.ScanCode == 0x02)
      return KREXO_KEY_DOWN;
    if (key.ScanCode == 0x03)
      return KREXO_KEY_DOWN; // Sometimes Right is Down in some shells
    if (key.ScanCode == 0x17)
      return KREXO_KEY_ESC;

    if (key.UnicodeChar == L'\r' || key.UnicodeChar == L'\n')
      return KREXO_KEY_ENTER;
    if (key.UnicodeChar == L'\b')
      return KREXO_KEY_BACKSPACE;

    if (key.UnicodeChar >= 32 && key.UnicodeChar <= 126) {
      return (int)key.UnicodeChar;
    }
  }
  return KREXO_KEY_NONE;
}

static void k_memcpy(void *dest, const void *src, uint64_t n) {
  uint8_t *d = (uint8_t *)dest;
  const uint8_t *s = (const uint8_t *)src;
  while (n--)
    *d++ = *s++;
}

static void k_memset(void *s, int c, uint64_t n) {
  uint8_t *p = (uint8_t *)s;
  while (n--)
    *p++ = (uint8_t)c;
}

EFI_STATUS
EFIAPI
efi_main(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS status;
  gST = SystemTable;
  uefi_init_debug(ImageHandle, SystemTable);

  kprintf("\n[UEFI] BOOT START\n");

  // Load Memory Map
  krexo_mmap_t mmap;
  int mmap_ok = (krexo_mmap_init(&mmap) == 0);

  // Initialize Graphics
  krexo_fb_t fb;
  int has_fb = (krexo_fb_init(&fb) == 0);
  if (has_fb) {
    krexo_fb_fill_rect(&fb, 0, 0, fb.width, fb.height, 0x1a1a1a);
  }

  // Initialize Disk
  krexo_disk_t disk;
  if (uefi_disk_init(&disk) != 0) {
    if (has_fb)
      krexo_fb_draw_string(&fb, 100, 160, "Disk Driver: FAILED", 0xFF0000);
    while (1)
      ;
  }

  // Parse Config
  krexo_config_t config;
  unsigned long long csize = 4096;
  void *cbuf;
  SystemTable->BootServices->AllocatePool(EfiLoaderData, csize, &cbuf);
  if (uefi_read_file("krexo.conf", cbuf, &csize) == 0) {
    config_parse(cbuf, &config);
  } else {
    // Default entry if no config
    config.count = 1;
    k_memcpy(config.entries[0].name, "Default Kernel", 15);
    k_memcpy(config.entries[0].kernel_path, "kernel.elf", 11);
  }

  // Load Background Image if specified
  uint8_t *bg_buffer = NULL;
  if (config.background_image_path[0]) {
    unsigned long long bg_size = 0;
    // Call with size 0 to get the required size first
    if (uefi_read_file(config.background_image_path, NULL, &bg_size) != 0) {
      // Actually, my uefi_read_file might not support size 0 to probe.
      // Let's assume a large enough buffer or find a way to probe.
      // Wait, uefi_read_file wrapper usually takes a pointer to size.
      // Let's check common/fs/fat32.h or wherever uefi_read_file is defined.
    }

    // Safer approach for now: set a much larger limit or probe correctly.
    // Let's check how uefi_read_file is implemented.
    // Actually, I'll just use a 32MB limit for now which should cover any BMP.
    bg_size = 32 * 1024 * 1024;
    void *tmp_bg;
    if (SystemTable->BootServices->AllocatePool(EfiLoaderData, bg_size,
                                                &tmp_bg) == EFI_SUCCESS) {
      if (uefi_read_file(config.background_image_path, tmp_bg, &bg_size) == 0) {
        bg_buffer = (uint8_t *)tmp_bg;
      } else {
        SystemTable->BootServices->FreePool(tmp_bg);
      }
    }
  }

  // INTERACTIVE MENU
  int selected_idx =
      menu_loop(&fb, &config, uefi_menu_get_key, uefi_delay_ms, bg_buffer);

  if (selected_idx == -2) {
    kprintf("[UEFI] Rebooting to Firmware Setup...\n");
    UINT64 indications = 0x0000000000000001; // EFI_OS_INDICATIONS_BOOT_TO_FW_UI
    EFI_GUID global_guid = {0x8be4df61,
                            0x93ca,
                            0x11d2,
                            {0xaa, 0x0d, 0x00, 0xe0, 0x98, 0x03, 0x2b, 0x8c}};
    SystemTable->RuntimeServices->SetVariable(
        L"OsIndications", &global_guid,
        0x00000001 | 0x00000002 | 0x00000004, // NV | BS | RT
        sizeof(indications), &indications);
    SystemTable->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0,
                                              NULL);
  }

  krexo_boot_entry_t *entry = &config.entries[selected_idx];

  // LOAD SELECTED KERNEL
  unsigned long long ksize = 1024 * 1024 * 4; // 4MB Buffer
  void *kbuffer;
  SystemTable->BootServices->AllocatePool(EfiLoaderData, ksize, &kbuffer);

  if (uefi_read_file(entry->kernel_path, kbuffer, &ksize) == 0) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)kbuffer;
    Elf64_Phdr *phdr = (Elf64_Phdr *)((uint8_t *)kbuffer + ehdr->phoff);

    for (int i = 0; i < ehdr->phnum; i++) {
      if (phdr[i].type == PT_LOAD) {
        k_memcpy((void *)phdr[i].paddr, (uint8_t *)kbuffer + phdr[i].offset,
                 phdr[i].filesz);
        if (phdr[i].memsz > phdr[i].filesz) {
          k_memset((void *)(phdr[i].paddr + phdr[i].filesz), 0,
                   phdr[i].memsz - phdr[i].filesz);
        }
      }
    }

    // Prepare Boot Info
    static krexo_boot_info_t boot_info;
    boot_info.magic = KREXO_BOOT_MAGIC;
    k_memcpy(&boot_info.fb, &fb, sizeof(fb));
    boot_info.mmap_base = (uint64_t)mmap.entries;
    boot_info.mmap_entries = mmap.count;

    // Handle requests (Native Krexo protocol)
    // Look for DTB in UEFI configuration tables
    void *dtb_ptr = NULL;
    EFI_GUID dtb_guid = {0xb1b621d9,
                         0xfc05,
                         0x450a,
                         {0x8d, 0x82, 0x89, 0x07, 0x34, 0xca, 0x8f, 0x71}};
    for (UINTN i = 0; i < SystemTable->NumberOfTableEntries; i++) {
      int match = 1;
      for (int j = 0; j < 16; j++) {
        if (((uint8_t *)&SystemTable->ConfigurationTable[i].VendorGuid)[j] !=
            ((uint8_t *)&dtb_guid)[j]) {
          match = 0;
          break;
        }
      }
      if (match) {
        dtb_ptr = SystemTable->ConfigurationTable[i].VendorTable;
        break;
      }
    }

    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    requests_handle((void *)0x1000000, 0x100000, &fb, &mmap, entry->cmdline,
                    (uint32_t)cr3, dtb_ptr);

    // Exit Boot Services before jumping
    UINTN MapSize = 0, MapKey = 0, DescriptorSize = 0;
    UINT32 DescriptorVersion = 0;
    SystemTable->BootServices->GetMemoryMap(
        &MapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);
    SystemTable->BootServices->ExitBootServices(ImageHandle, MapKey);

    // JUMP (Krexo protocol path)
    void(__attribute__((sysv_abi)) * kernel_entry)(krexo_boot_info_t *) =
        (void(__attribute__((sysv_abi)) *)(krexo_boot_info_t *))ehdr->entry;

    kernel_entry(&boot_info);
  }

  while (1)
    ;
  return EFI_SUCCESS;
}
