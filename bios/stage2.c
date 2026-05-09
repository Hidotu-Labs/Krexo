#include <common/kprint.h>
#include <common/mmap.h>
#include <common/fb.h>
#include <common/disk.h>
#include <common/fs/fat32.h>
#include <common/elf.h>
#include <common/boot_info.h>
#include <stdint.h>
#include <stddef.h>

#include <common/config.h>
#include <common/menu.h>
#include <common/request_handler.h>

extern int bios_disk_init(krexo_disk_t *disk, uint8_t drive_id);
extern void switch_to_long_mode(uint32_t pml4_addr, uint32_t entry_point, uint32_t boot_info_ptr);

static uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void bios_delay_ms(int ms) {
    // Use PIT Channel 2 (PC Speaker timer) for reliable delay
    // 1.193182 MHz fixed frequency
    uint8_t val = inb(0x61);
    val &= 0xFD; // Speaker off, but timer running
    for (int i = 0; i < ms; i++) {
        // Init PIT Channel 2, Mode 0 (interrupt on terminal count)
        // Set count to ~1ms (1193 ticks)
        __asm__ volatile ("outb %%al, $0x43" : : "a"((uint8_t)0xB0));
        __asm__ volatile ("outb %%al, $0x42" : : "a"((uint8_t)(1193 & 0xFF)));
        __asm__ volatile ("outb %%al, $0x42" : : "a"((uint8_t)(1193 >> 8)));
        
        // Wait for bit 5 of port 0x61 to go high
        while (!(inb(0x61) & 0x20));
    }
}

static int bios_get_key(void) {
    // Check if data is available
    if (!(inb(0x64) & 1)) return 0;
    
    uint8_t scan = inb(0x60);
    if (scan == 0x48) return 1; // Up Arrow
    if (scan == 0x50) return 2; // Down Arrow
    if (scan == 0x1C) return 3; // Enter
    return 0;
}

static void* k_memcpy(void* dest, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) *d++ = *s++;
    return dest;
}

static void* k_memset(void* s, int c, uint32_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

void debug_putc(char c) {
    __asm__ volatile("outb %0, $0xE9" : : "a"((uint8_t)c));
}

void debug_print(const char* s) {
    while (*s) debug_putc(*s++);
}

#define PML4_ADDR 0x70000
#define PDPT_ADDR 0x71000
#define PD_BASE   0x72000

void setup_long_mode_paging() {
    uint64_t* pml4 = (uint64_t*)PML4_ADDR;
    uint64_t* pdpt = (uint64_t*)PDPT_ADDR;
    
    k_memset((void*)PML4_ADDR, 0, 0x1000 * 6);
    
    pml4[0] = (uint32_t)(uintptr_t)pdpt | 0x03;
    for (uint32_t i = 0; i < 4; i++) {
        uint64_t* pd = (uint64_t*)(PD_BASE + (i * 0x1000));
        pdpt[i] = (uint32_t)(uintptr_t)pd | 0x03;
        for (uint32_t j = 0; j < 512; j++) {
            pd[j] = ((uint64_t)(i * 512 + j) * 0x200000) | 0x83;
        }
    }
}

void stage2_main(uint32_t boot_drive) {
  setup_long_mode_paging();

  krexo_fb_t fb;
  if (krexo_fb_init(&fb) != 0) {
      debug_print("ERROR: VBE Init failed! No hardware info.\n");
      while(1);
  }

  krexo_fb_fill_rect(&fb, 0, 0, fb.width, fb.height, 0x1a1a1a);
  const char* title = "KREXO BOOTLOADER v0.1 (BIOS)";
  int title_len = 0; while(title[title_len]) title_len++;
  krexo_fb_draw_string(&fb, (fb.width - (title_len * 8)) / 2, 100, title, 0xFFFFFF);
  
  krexo_mmap_t mmap;
  krexo_mmap_init(&mmap);

  krexo_disk_t disk;
  if (bios_disk_init(&disk, (uint8_t)boot_drive) == 0) {
      if (fat32_init(&disk) == 0) {
          // Parse Config
          krexo_config_t config;
          uint32_t conf_cluster, conf_size;
          if (fat32_find_file("krexo.conf", &conf_cluster, &conf_size) == 0) {
              void* conf_buf = (void*)0x3000000;
              fat32_read_file(conf_cluster, conf_buf, conf_size);
              config_parse(conf_buf, &config);
          } else {
              config.count = 1;
              k_memcpy(config.entries[0].name, "BIOS Default", 13);
              k_memcpy(config.entries[0].kernel_path, "kernel.elf", 11);
          }

          // INTERACTIVE MENU
          int selected_idx = menu_loop(&fb, &config, bios_get_key, bios_delay_ms);
          krexo_boot_entry_t* entry = &config.entries[selected_idx];

          uint32_t cluster, size;
          if (fat32_find_file(entry->kernel_path, &cluster, &size) == 0) {
              void* temp_buffer = (void*)0x2000000;
              fat32_read_file(cluster, temp_buffer, size);
              
              Elf64_Ehdr* ehdr = (Elf64_Ehdr*)temp_buffer;
              Elf64_Phdr* phdr = (Elf64_Phdr*)((uint8_t*)temp_buffer + ehdr->phoff);
              
              for (int i = 0; i < ehdr->phnum; i++) {
                  if (phdr[i].type == PT_LOAD) {
                      k_memcpy((void*)(uintptr_t)phdr[i].paddr, (uint8_t*)temp_buffer + phdr[i].offset, phdr[i].filesz);
                      if (phdr[i].memsz > phdr[i].filesz) {
                          k_memset((void*)(uintptr_t)(phdr[i].paddr + phdr[i].filesz), 0, phdr[i].memsz - phdr[i].filesz);
                      }
                  }
              }

              krexo_boot_info_t* boot_info = (krexo_boot_info_t*)0x1000;
              boot_info->magic = KREXO_BOOT_MAGIC;
              k_memcpy(&boot_info->fb, &fb, sizeof(fb));
              boot_info->mmap_base = 0x5000;
              boot_info->mmap_entries = mmap.count;

              // SCAN FOR KREXO REQUESTS (Scanning first 1MB of loaded kernel)
              requests_handle((void*)0x1000000, 0x100000, &fb);

              debug_print("Jumping to ");
              debug_print(entry->name);
              debug_print("...\n");
              switch_to_long_mode(PML4_ADDR, (uint32_t)ehdr->entry, (uint32_t)boot_info);
          }
      }
  }

  while (1);
}
