#include <common/fb.h>
#include <common/kprint.h>
#include <common/limine_compat.h>
#include <common/mmap.h>
#include <stddef.h>

// Bump allocator for response structures
static uint64_t limine_alloc_ptr = 0x90000;

static void *limine_alloc(size_t size) {
  // Align to 8 bytes
  limine_alloc_ptr = (limine_alloc_ptr + 7) & ~7ULL;
  void *ptr = (void *)(uintptr_t)limine_alloc_ptr;
  limine_alloc_ptr += size;
  return ptr;
}

static char *limine_strdup(const char *s) {
  size_t len = 0;
  while (s[len])
    len++;
  char *buf = limine_alloc(len + 1);
  for (size_t i = 0; i < len; i++)
    buf[i] = s[i];
  buf[len] = 0;
  return buf;
}

// Limine protocol constants
#define LIMINE_MAGIC_0 0xc7b1dd30df4c8b88ULL
#define LIMINE_MAGIC_1 0x0a82e883a194f07bULL

// Base revision tag magic
#define LIMINE_BASE_REV_MAGIC_0 0xf9562b2d5c95a6c8ULL
#define LIMINE_BASE_REV_MAGIC_1 0x6a7b384944536bdcULL

// Limine request start/end markers
#define LIMINE_START_0 0xf6b8f4b39de7d1aeULL
#define LIMINE_START_1 0xfab91a6940fcb9cfULL
#define LIMINE_START_2 0x785c6ed015d3e316ULL
#define LIMINE_START_3 0x181e920a7852b9d9ULL
#define LIMINE_END_0   0xadc0e0531bb10d03ULL
#define LIMINE_END_1   0x9572709f31764c62ULL

// Request IDs (last 2 of the 4-word ID)
#define LIMINE_BOOTINFO_ID0    0xf55038d8e2a1202fULL
#define LIMINE_BOOTINFO_ID1    0x279426fcf5f59740ULL

#define LIMINE_FB_ID0          0x9d5827dcd881dd75ULL
#define LIMINE_FB_ID1          0xa3148604f6fab11bULL

#define LIMINE_MEMMAP_ID0      0x67cf3d9d378a806fULL
#define LIMINE_MEMMAP_ID1      0xe304acdfc50c3c62ULL

#define LIMINE_HHDM_ID0        0x48dcf1cb8ad2b852ULL
#define LIMINE_HHDM_ID1        0x63984e959a98244bULL

#define LIMINE_ENTRY_POINT_ID0 0x13d86c035a1cd3e1ULL
#define LIMINE_ENTRY_POINT_ID1 0x2b0caa89d8f3026aULL

#define LIMINE_EXEC_ADDR_ID0   0x71ba76863cc55f63ULL
#define LIMINE_EXEC_ADDR_ID1   0xb2644a48c516a487ULL

#define LIMINE_FIRMWARE_ID0    0x8c2f75d90bef28a8ULL
#define LIMINE_FIRMWARE_ID1    0x7045a4688eac00c3ULL

#define LIMINE_PAGING_ID0      0x95c1a0edab0944cbULL
#define LIMINE_PAGING_ID1      0xa4e5cb3842f7488aULL

#define LIMINE_CMDLINE_ID0     0x4b161536e598651eULL
#define LIMINE_CMDLINE_ID1     0xb390ad4a2f1f303aULL

#define LIMINE_EXEC_FILE_ID0   0xad97e90e83f1ed67ULL
#define LIMINE_EXEC_FILE_ID1   0x31eb5d1c5ff23b69ULL

#define LIMINE_RSDP_ID0        0xc5e77b6b397e7b43ULL
#define LIMINE_RSDP_ID1        0x27637845accdcf3cULL

// --- Limine memory map types ---
#define LIMINE_MEMMAP_USABLE                 0
#define LIMINE_MEMMAP_RESERVED               1
#define LIMINE_MEMMAP_ACPI_RECLAIMABLE       2
#define LIMINE_MEMMAP_ACPI_NVS               3
#define LIMINE_MEMMAP_BAD_MEMORY             4
#define LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE 5
#define LIMINE_MEMMAP_EXECUTABLE_AND_MODULES 6
#define LIMINE_MEMMAP_FRAMEBUFFER            7

// Convert Krexo mem type to Limine mem type
static uint64_t krexo_to_limine_memtype(int krexo_type) {
  switch (krexo_type) {
  case 1: // KREXO_MEM_CONVENTIONAL
    return LIMINE_MEMMAP_USABLE;
  case 2: // KREXO_MEM_BOOT_DATA
    return LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE;
  case 4: // KREXO_MEM_ACPI_RECLAIM
    return LIMINE_MEMMAP_ACPI_RECLAIMABLE;
  case 5: // KREXO_MEM_ACPI_NVS
    return LIMINE_MEMMAP_ACPI_NVS;
  case 6: // KREXO_MEM_UNUSABLE
    return LIMINE_MEMMAP_BAD_MEMORY;
  default:
    return LIMINE_MEMMAP_RESERVED;
  }
}

// Find RSDP in BIOS memory areas
static void *find_rsdp(void) {
  // Search in EBDA (Extended BIOS Data Area)
  uint16_t ebda_seg = *(uint16_t *)0x40E;
  uint8_t *ebda = (uint8_t *)((uintptr_t)ebda_seg << 4);
  for (int i = 0; i < 1024; i += 16) {
    if (ebda[i] == 'R' && ebda[i + 1] == 'S' && ebda[i + 2] == 'D' &&
        ebda[i + 3] == ' ' && ebda[i + 4] == 'P' && ebda[i + 5] == 'T' &&
        ebda[i + 6] == 'R' && ebda[i + 7] == ' ') {
      return &ebda[i];
    }
  }
  // Search in BIOS ROM area (0xE0000 - 0xFFFFF)
  for (uint8_t *p = (uint8_t *)0xE0000; p < (uint8_t *)0x100000; p += 16) {
    if (p[0] == 'R' && p[1] == 'S' && p[2] == 'D' && p[3] == ' ' &&
        p[4] == 'P' && p[5] == 'T' && p[6] == 'R' && p[7] == ' ') {
      return p;
    }
  }
  return (void *)0;
}

int limine_detect(void *kernel_start, size_t kernel_size) {
  uint64_t *ptr = (uint64_t *)kernel_start;
  size_t count = kernel_size / 8;

  for (size_t i = 0; i < count - 3; i++) {
    // Check for Limine common magic in request format
    if (ptr[i] == LIMINE_MAGIC_0 && ptr[i + 1] == LIMINE_MAGIC_1) {
      return 1;  // Limine protocol detected
    }
    // Check for base revision tag
    if (ptr[i] == LIMINE_BASE_REV_MAGIC_0 &&
        ptr[i + 1] == LIMINE_BASE_REV_MAGIC_1) {
      return 1;
    }
  }
  return 0;
}

void limine_handle_requests(void *kernel_start, size_t kernel_size,
                            krexo_fb_t *fb, krexo_mmap_t *mmap,
                            const char *cmdline, uint64_t kernel_phys_base,
                            uint64_t *out_entry) {
  uint64_t *ptr = (uint64_t *)kernel_start;
  size_t count = kernel_size / 8;

  // Default entry point — will be overridden if entry point request is found
  if (out_entry)
    *out_entry = 0;

  // 1. Find and handle base revision tag
  for (size_t i = 0; i < count - 2; i++) {
    if (ptr[i] == LIMINE_BASE_REV_MAGIC_0 &&
        ptr[i + 1] == LIMINE_BASE_REV_MAGIC_1) {
      uint64_t requested_rev = ptr[i + 2];
      kprintf("[LIMINE] Base revision %d requested\n", (int)requested_rev);
      // We support revision 2 (current). Signal support by zeroing it.
      if (requested_rev <= 3) {
        ptr[i + 2] = 0;
        kprintf("[LIMINE] Base revision acknowledged\n");
      }
      break;
    }
  }

  // 2. Find request boundaries (optional markers)
  size_t scan_start = 0;
  size_t scan_end = count - 5;

  for (size_t i = 0; i < count - 3; i++) {
    if (ptr[i] == LIMINE_START_0 && ptr[i + 1] == LIMINE_START_1 &&
        ptr[i + 2] == LIMINE_START_2 && ptr[i + 3] == LIMINE_START_3) {
      scan_start = i + 4;
      kprintf("[LIMINE] Start marker found at offset 0x%x\n",
              (uint32_t)(i * 8));
    }
    if (i < count - 1 && ptr[i] == LIMINE_END_0 &&
        ptr[i + 1] == LIMINE_END_1) {
      scan_end = i;
      kprintf("[LIMINE] End marker found at offset 0x%x\n",
              (uint32_t)(i * 8));
      break;
    }
  }

  // 3. Scan for requests
  // Limine request layout:
  //   [0] LIMINE_COMMON_MAGIC word 0
  //   [1] LIMINE_COMMON_MAGIC word 1
  //   [2] request-specific ID word 0
  //   [3] request-specific ID word 1
  //   [4] revision
  //   [5] response pointer (initially 0)
  //   [6+] request-specific data

  for (size_t i = scan_start; i < scan_end; i++) {
    if (ptr[i] != LIMINE_MAGIC_0 || ptr[i + 1] != LIMINE_MAGIC_1)
      continue;

    uint64_t req_id0 = ptr[i + 2];
    uint64_t req_id1 = ptr[i + 3];
    // uint64_t req_rev = ptr[i + 4];
    // ptr[i + 5] = response pointer

    // --- Bootloader Info ---
    if (req_id0 == LIMINE_BOOTINFO_ID0 && req_id1 == LIMINE_BOOTINFO_ID1) {
      // struct: { revision, name_ptr, version_ptr }
      typedef struct {
        uint64_t revision;
        uint64_t name;
        uint64_t version;
      } resp_t;
      resp_t *resp = limine_alloc(sizeof(resp_t));
      resp->revision = 0;
      resp->name = (uint64_t)(uintptr_t)limine_strdup("Krexo");
      resp->version = (uint64_t)(uintptr_t)limine_strdup("0.1");
      ptr[i + 5] = (uint64_t)(uintptr_t)resp;
      kprintf("[LIMINE] Bootloader info request fulfilled\n");
    }

    // --- Framebuffer ---
    else if (req_id0 == LIMINE_FB_ID0 && req_id1 == LIMINE_FB_ID1) {
      if (fb) {
        // Build a limine_framebuffer struct
        // struct limine_framebuffer: address, width, height, pitch, bpp,
        //   memory_model, masks, unused[7], edid_size, edid, mode_count, modes
        typedef struct {
          uint64_t address;
          uint64_t width;
          uint64_t height;
          uint64_t pitch;
          uint16_t bpp;
          uint8_t memory_model;
          uint8_t red_mask_size;
          uint8_t red_mask_shift;
          uint8_t green_mask_size;
          uint8_t green_mask_shift;
          uint8_t blue_mask_size;
          uint8_t blue_mask_shift;
          uint8_t unused[7];
          uint64_t edid_size;
          uint64_t edid;
          uint64_t mode_count;
          uint64_t modes;
        } limine_fb_t;

        typedef struct {
          uint64_t revision;
          uint64_t framebuffer_count;
          uint64_t framebuffers; // ptr to array of ptrs
        } fb_resp_t;

        limine_fb_t *lfb = limine_alloc(sizeof(limine_fb_t));
        lfb->address = fb->base;
        lfb->width = fb->width;
        lfb->height = fb->height;
        lfb->pitch = fb->pitch;
        lfb->bpp = fb->bpp;
        lfb->memory_model = 1; // RGB
        lfb->red_mask_size = 8;
        lfb->red_mask_shift = 16;
        lfb->green_mask_size = 8;
        lfb->green_mask_shift = 8;
        lfb->blue_mask_size = 8;
        lfb->blue_mask_shift = 0;
        for (int j = 0; j < 7; j++)
          lfb->unused[j] = 0;
        lfb->edid_size = 0;
        lfb->edid = 0;
        lfb->mode_count = 0;
        lfb->modes = 0;

        // Array of pointers to framebuffers
        uint64_t *fb_ptrs = limine_alloc(sizeof(uint64_t));
        fb_ptrs[0] = (uint64_t)(uintptr_t)lfb;

        fb_resp_t *resp = limine_alloc(sizeof(fb_resp_t));
        resp->revision = 0;
        resp->framebuffer_count = 1;
        resp->framebuffers = (uint64_t)(uintptr_t)fb_ptrs;

        ptr[i + 5] = (uint64_t)(uintptr_t)resp;
        kprintf("[LIMINE] Framebuffer request fulfilled (%dx%d)\n",
                fb->width, fb->height);
      }
    }

    // --- Memory Map ---
    else if (req_id0 == LIMINE_MEMMAP_ID0 && req_id1 == LIMINE_MEMMAP_ID1) {
      if (mmap && mmap->count > 0) {
        // Build Limine memmap entries (each: base, length, type — all uint64)
        typedef struct {
          uint64_t base;
          uint64_t length;
          uint64_t type;
        } limine_mmap_entry_t;

        typedef struct {
          uint64_t revision;
          uint64_t entry_count;
          uint64_t entries; // ptr to array of ptrs
        } mmap_resp_t;

        size_t n = mmap->count;
        limine_mmap_entry_t **entry_ptrs = limine_alloc(n * sizeof(uint64_t));

        for (size_t j = 0; j < n; j++) {
          limine_mmap_entry_t *e = limine_alloc(sizeof(limine_mmap_entry_t));
          e->base = mmap->entries[j].base;
          e->length = mmap->entries[j].length;
          e->type = krexo_to_limine_memtype(mmap->entries[j].type);
          entry_ptrs[j] = e;
        }

        mmap_resp_t *resp = limine_alloc(sizeof(mmap_resp_t));
        resp->revision = 0;
        resp->entry_count = n;
        resp->entries = (uint64_t)(uintptr_t)entry_ptrs;

        ptr[i + 5] = (uint64_t)(uintptr_t)resp;
        kprintf("[LIMINE] Memory map request fulfilled (%d entries)\n",
                (int)n);
      }
    }

    // --- HHDM ---
    else if (req_id0 == LIMINE_HHDM_ID0 && req_id1 == LIMINE_HHDM_ID1) {
      typedef struct {
        uint64_t revision;
        uint64_t offset;
      } hhdm_resp_t;

      hhdm_resp_t *resp = limine_alloc(sizeof(hhdm_resp_t));
      resp->revision = 0;
      resp->offset = 0xffff800000000000ULL;
      ptr[i + 5] = (uint64_t)(uintptr_t)resp;
      kprintf("[LIMINE] HHDM request fulfilled\n");
    }

    // --- Executable Address (Kernel Address) ---
    else if (req_id0 == LIMINE_EXEC_ADDR_ID0 &&
             req_id1 == LIMINE_EXEC_ADDR_ID1) {
      typedef struct {
        uint64_t revision;
        uint64_t physical_base;
        uint64_t virtual_base;
      } exec_addr_resp_t;

      exec_addr_resp_t *resp = limine_alloc(sizeof(exec_addr_resp_t));
      resp->revision = 0;
      resp->physical_base = kernel_phys_base;
      resp->virtual_base = kernel_phys_base; // Identity mapped for now
      ptr[i + 5] = (uint64_t)(uintptr_t)resp;
      kprintf("[LIMINE] Executable address request fulfilled (phys=0x%x)\n",
              (uint32_t)kernel_phys_base);
    }

    // --- Firmware Type ---
    else if (req_id0 == LIMINE_FIRMWARE_ID0 &&
             req_id1 == LIMINE_FIRMWARE_ID1) {
      typedef struct {
        uint64_t revision;
        uint64_t firmware_type;
      } fw_resp_t;

      fw_resp_t *resp = limine_alloc(sizeof(fw_resp_t));
      resp->revision = 0;
#ifdef __UEFI__
      resp->firmware_type = 2; // EFI64
#else
      resp->firmware_type = 0; // X86 BIOS
#endif
      ptr[i + 5] = (uint64_t)(uintptr_t)resp;
      kprintf("[LIMINE] Firmware type request fulfilled\n");
    }

    // --- Paging Mode ---
    else if (req_id0 == LIMINE_PAGING_ID0 && req_id1 == LIMINE_PAGING_ID1) {
      typedef struct {
        uint64_t revision;
        uint64_t mode;
      } paging_resp_t;

      paging_resp_t *resp = limine_alloc(sizeof(paging_resp_t));
      resp->revision = 0;
      resp->mode = 0; // 4-level paging
      ptr[i + 5] = (uint64_t)(uintptr_t)resp;
      kprintf("[LIMINE] Paging mode request fulfilled (4-level)\n");
    }

    // --- Entry Point ---
    else if (req_id0 == LIMINE_ENTRY_POINT_ID0 &&
             req_id1 == LIMINE_ENTRY_POINT_ID1) {
      typedef struct {
        uint64_t revision;
      } ep_resp_t;

      // The entry point is at ptr[i+6]
      if (out_entry && ptr[i + 6] != 0) {
        *out_entry = ptr[i + 6];
        kprintf("[LIMINE] Entry point override: 0x%x\n",
                (uint32_t)ptr[i + 6]);
      }

      ep_resp_t *resp = limine_alloc(sizeof(ep_resp_t));
      resp->revision = 0;
      ptr[i + 5] = (uint64_t)(uintptr_t)resp;
    }

    // --- Executable Command Line ---
    else if (req_id0 == LIMINE_CMDLINE_ID0 &&
             req_id1 == LIMINE_CMDLINE_ID1) {
      typedef struct {
        uint64_t revision;
        uint64_t cmdline;
      } cmdline_resp_t;

      cmdline_resp_t *resp = limine_alloc(sizeof(cmdline_resp_t));
      resp->revision = 0;
      resp->cmdline =
          (uint64_t)(uintptr_t)limine_strdup(cmdline ? cmdline : "");
      ptr[i + 5] = (uint64_t)(uintptr_t)resp;
      kprintf("[LIMINE] Cmdline request fulfilled\n");
    }

    // --- Executable File ---
    else if (req_id0 == LIMINE_EXEC_FILE_ID0 &&
             req_id1 == LIMINE_EXEC_FILE_ID1) {
      // Provide a minimal limine_file struct
      typedef struct {
        uint64_t revision;
        uint64_t address;
        uint64_t size;
        uint64_t path;
        uint64_t string;
        uint32_t media_type;
        uint32_t unused;
        uint32_t tftp_ip;
        uint32_t tftp_port;
        uint32_t partition_index;
        uint32_t mbr_disk_id;
        uint8_t  guids[48]; // 3x limine_uuid
      } limine_file_t;

      typedef struct {
        uint64_t revision;
        uint64_t executable_file;
      } exec_file_resp_t;

      limine_file_t *file = limine_alloc(sizeof(limine_file_t));
      file->revision = 0;
      file->address = (uint64_t)(uintptr_t)kernel_start;
      file->size = kernel_size;
      file->path = (uint64_t)(uintptr_t)limine_strdup("/kernel.elf");
      file->string = (uint64_t)(uintptr_t)limine_strdup("");
      file->media_type = 0;

      exec_file_resp_t *resp = limine_alloc(sizeof(exec_file_resp_t));
      resp->revision = 0;
      resp->executable_file = (uint64_t)(uintptr_t)file;
      ptr[i + 5] = (uint64_t)(uintptr_t)resp;
      kprintf("[LIMINE] Executable file request fulfilled\n");
    }

    // --- RSDP ---
    else if (req_id0 == LIMINE_RSDP_ID0 && req_id1 == LIMINE_RSDP_ID1) {
      typedef struct {
        uint64_t revision;
        uint64_t address;
      } rsdp_resp_t;

      void *rsdp = find_rsdp();
      if (rsdp) {
        rsdp_resp_t *resp = limine_alloc(sizeof(rsdp_resp_t));
        resp->revision = 0;
        resp->address = (uint64_t)(uintptr_t)rsdp;
        ptr[i + 5] = (uint64_t)(uintptr_t)resp;
        kprintf("[LIMINE] RSDP request fulfilled (addr=0x%x)\n",
                (uint32_t)(uintptr_t)rsdp);
      }
    }
  }
}
