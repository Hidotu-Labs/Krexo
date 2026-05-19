#include <common/mmap.h>
#include <common/kprint.h>
#include <stdint.h>

// BIOS E820 entry format (as stored by stage1.asm)
typedef struct {
  uint32_t base_low;
  uint32_t base_high;
  uint32_t length_low;
  uint32_t length_high;
  uint32_t type;
  uint32_t acpi_attrs;  // Extended attributes (ACPI 3.0+)
} __attribute__((packed)) bios_e820_entry_t;

// Memory map is stored at 0x7000 by stage1
#define BIOS_MMAP_ADDR 0x7000
#define BIOS_MMAP_MAX_ENTRIES 64

// BIOS E820 memory types
#define BIOS_E820_USABLE        1
#define BIOS_E820_RESERVED      2
#define BIOS_E820_ACPI_RECLAIM  3
#define BIOS_E820_ACPI_NVS      4
#define BIOS_E820_UNUSABLE      5

// Convert BIOS E820 type to Krexo unified type
static krexo_mem_type_t bios_to_krexo_type(uint32_t bios_type) {
  switch (bios_type) {
    case BIOS_E820_USABLE:
      return KREXO_MEM_CONVENTIONAL;
    case BIOS_E820_ACPI_RECLAIM:
      return KREXO_MEM_ACPI_RECLAIM;
    case BIOS_E820_ACPI_NVS:
      return KREXO_MEM_ACPI_NVS;
    case BIOS_E820_UNUSABLE:
      return KREXO_MEM_UNUSABLE;
    default:
      return KREXO_MEM_RESERVED;
  }
}

// Static storage for unified entries
static krexo_mmap_entry_t g_entries[BIOS_MMAP_MAX_ENTRIES];

int krexo_mmap_init(krexo_mmap_t *mmap) {
  // Read memory map from location stored by stage1
  // First byte is the entry count
  volatile uint8_t *bios_mmap = (volatile uint8_t *)BIOS_MMAP_ADDR;
  uint8_t entry_count = bios_mmap[0];
  
  if (entry_count == 0 || entry_count > BIOS_MMAP_MAX_ENTRIES) {
    return -1;
  }
  
  // Entries start after the count byte
  bios_e820_entry_t *bios_entries = (bios_e820_entry_t *)(BIOS_MMAP_ADDR + 16);
  
  mmap->entries = g_entries;
  mmap->capacity = BIOS_MMAP_MAX_ENTRIES;
  mmap->count = 0;
  
  for (uint8_t i = 0; i < entry_count && i < BIOS_MMAP_MAX_ENTRIES; i++) {
    krexo_mmap_entry_t *entry = &mmap->entries[mmap->count];
    bios_e820_entry_t *bios = &bios_entries[i];
    
    entry->base = ((uint64_t)bios->base_high << 32) | bios->base_low;
    entry->length = ((uint64_t)bios->length_high << 32) | bios->length_low;
    entry->type = bios_to_krexo_type(bios->type);
    entry->flags = KREXO_MEM_FLAG_NONE;
    
    // If ACPI attributes present and entry is marked as non-volatile
    if (bios->acpi_attrs & 0x02) {
      entry->flags |= KREXO_MEM_FLAG_NONVOLATILE;
    }
    
    // Skip zero-length entries
    if (entry->length > 0) {
      mmap->count++;
    }
  }
  
  return 0;
}

void krexo_mmap_free(krexo_mmap_t *mmap) {
  mmap->entries = NULL;
  mmap->count = 0;
  mmap->capacity = 0;
}

uint64_t krexo_mmap_total_usable(const krexo_mmap_t *mmap) {
  uint64_t total = 0;
  for (size_t i = 0; i < mmap->count; i++) {
    if (mmap->entries[i].type == KREXO_MEM_CONVENTIONAL ||
        mmap->entries[i].type == KREXO_MEM_BOOT_DATA) {
      total += mmap->entries[i].length;
    }
  }
  return total;
}

uint64_t krexo_mmap_total_free(const krexo_mmap_t *mmap) {
  uint64_t total = 0;
  for (size_t i = 0; i < mmap->count; i++) {
    if (mmap->entries[i].type == KREXO_MEM_CONVENTIONAL) {
      total += mmap->entries[i].length;
    }
  }
  return total;
}

static const char *mem_type_str(krexo_mem_type_t type) {
  switch (type) {
    case KREXO_MEM_RESERVED:      return "Reserved";
    case KREXO_MEM_CONVENTIONAL:  return "Conventional";
    case KREXO_MEM_BOOT_DATA:     return "Boot Data";
    case KREXO_MEM_RUNTIME_CODE:  return "Runtime Code";
    case KREXO_MEM_RUNTIME_DATA:  return "Runtime Data";
    case KREXO_MEM_ACPI_RECLAIM:  return "ACPI Reclaim";
    case KREXO_MEM_ACPI_NVS:      return "ACPI NVS";
    case KREXO_MEM_UNUSABLE:      return "Unusable";
    case KREXO_MEM_MMIO:          return "MMIO";
    case KREXO_MEM_PERSISTENT:    return "Persistent";
    default:                      return "Unknown";
  }
}

void krexo_mmap_print(const krexo_mmap_t *mmap) {
  kprintf("\nMemory Map (%d entries):\n", mmap->count);
  kprintf("Base              Length            Type\n");
  kprintf("----------------  ----------------  ----\n");
  
  for (size_t i = 0; i < mmap->count; i++) {
    const krexo_mmap_entry_t *e = &mmap->entries[i];
    kprintf("0x%016llx 0x%016llx %s\n", e->base, e->length, mem_type_str(e->type));
  }
  
  uint64_t usable = krexo_mmap_total_usable(mmap);
  uint64_t free_mem = krexo_mmap_total_free(mmap);
  kprintf("\nTotal usable: 0x%llx bytes\n", usable);
  kprintf("Total free:   0x%llx bytes\n", free_mem);
}
