#pragma once
#include <stdint.h>
#include <common/fb.h>

typedef struct {
    uint32_t magic;         // "KREX"
    uint32_t _reserved;     // Padding to align the struct
    krexo_fb_t fb;          
    uint64_t mmap_base;     // Forced 64-bit
    uint32_t mmap_entries;
} krexo_boot_info_t;

#define KREXO_BOOT_MAGIC 0x5845524B // "KREX"
