#include <Uefi.h>
#include <common/kprint.h>
#include <common/mmap.h>
#include <common/fb.h>
#include <common/disk.h>
#include <common/fs/fat32.h>
#include <common/request_handler.h>
#include <common/elf.h>
#include <common/boot_info.h>
#include <common/config.h>
#include <common/menu.h>

extern int uefi_disk_init(krexo_disk_t *disk);
void uefi_init_debug(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);
extern int uefi_read_file(const char* filename, void* buffer, unsigned long long* size);

static EFI_SYSTEM_TABLE *gST;

static void uefi_delay_ms(int ms) {
    gST->BootServices->Stall(ms * 1000);
}

static int uefi_menu_get_key(void) {
    EFI_INPUT_KEY key;
    EFI_STATUS status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key);
    if (status == EFI_SUCCESS) {
        if (key.ScanCode == 0x01) return 1; // Up
        if (key.ScanCode == 0x02) return 2; // Down
        if (key.UnicodeChar == L'\r' || key.UnicodeChar == L'\n') return 3; // Enter
    }
    return 0;
}

static void k_memcpy(void* dest, const void* src, uint64_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) *d++ = *s++;
}

static void k_memset(void* s, int c, uint64_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) *p++ = (uint8_t)c;
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
    const char* title = "KREXO BOOTLOADER v0.1 (UEFI)";
    int title_len = 0; while(title[title_len]) title_len++;
    krexo_fb_draw_string(&fb, (fb.width - (title_len * 8)) / 2, 100, title, 0xFFFFFF);
  }

  // Initialize Disk
  krexo_disk_t disk;
  if (uefi_disk_init(&disk) != 0) {
      if (has_fb) krexo_fb_draw_string(&fb, 100, 160, "Disk Driver: FAILED", 0xFF0000);
      while(1);
  }

  // Parse Config
  krexo_config_t config;
  unsigned long long csize = 4096;
  void* cbuf;
  SystemTable->BootServices->AllocatePool(EfiLoaderData, csize, &cbuf);
  if (uefi_read_file("krexo.conf", cbuf, &csize) == 0) {
      config_parse(cbuf, &config);
  } else {
      // Default entry if no config
      config.count = 1;
      k_memcpy(config.entries[0].name, "Default Kernel", 15);
      k_memcpy(config.entries[0].kernel_path, "kernel.elf", 11);
  }

  // INTERACTIVE MENU
  int selected_idx = menu_loop(&fb, &config, uefi_menu_get_key, uefi_delay_ms);
  krexo_boot_entry_t* entry = &config.entries[selected_idx];

  // LOAD SELECTED KERNEL
  unsigned long long ksize = 1024 * 1024 * 4; // 4MB Buffer
  void* kbuffer;
  SystemTable->BootServices->AllocatePool(EfiLoaderData, ksize, &kbuffer);

  if (uefi_read_file(entry->kernel_path, kbuffer, &ksize) == 0) {
      Elf64_Ehdr* ehdr = (Elf64_Ehdr*)kbuffer;
      Elf64_Phdr* phdr = (Elf64_Phdr*)((uint8_t*)kbuffer + ehdr->phoff);

      for (int i = 0; i < ehdr->phnum; i++) {
          if (phdr[i].type == PT_LOAD) {
              k_memcpy((void*)phdr[i].paddr, (uint8_t*)kbuffer + phdr[i].offset, phdr[i].filesz);
              if (phdr[i].memsz > phdr[i].filesz) {
                  k_memset((void*)(phdr[i].paddr + phdr[i].filesz), 0, phdr[i].memsz - phdr[i].filesz);
              }
          }
      }

      // Prepare Boot Info
      static krexo_boot_info_t boot_info;
      boot_info.magic = KREXO_BOOT_MAGIC;
      k_memcpy(&boot_info.fb, &fb, sizeof(fb));
      boot_info.mmap_base = (uint64_t)mmap.entries;
      boot_info.mmap_entries = mmap.count;
      
      // SCAN FOR KREXO REQUESTS
      requests_handle((void*)0x1000000, 0x100000, &fb);

      // GET THE ABSOLUTE LAST KEY AND EXIT
      UINTN MapSize = 0;
      UINTN MapKey = 0;
      UINTN DescriptorSize = 0;
      UINT32 DescriptorVersion = 0;
      SystemTable->BootServices->GetMemoryMap(&MapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);
      SystemTable->BootServices->ExitBootServices(ImageHandle, MapKey);

      // JUMP!
      void (__attribute__((sysv_abi)) *kernel_entry)(krexo_boot_info_t*) = 
          (void (__attribute__((sysv_abi)) *)(krexo_boot_info_t*))ehdr->entry;

      kernel_entry(&boot_info);
  }

  while (1);
  return EFI_SUCCESS;
}
