#ifndef KREXO_MMAP_H
#define KREXO_MMAP_H

#include <stdint.h>
#include <stddef.h>

// Unified memory types - platform agnostic
typedef enum {
  KREXO_MEM_RESERVED,      // Unusable memory
  KREXO_MEM_CONVENTIONAL,  // Free RAM
  KREXO_MEM_BOOT_DATA,     // Boot loader data
  KREXO_MEM_RUNTIME_CODE,  // Runtime executable code (UEFI)
  KREXO_MEM_RUNTIME_DATA,  // Runtime data (UEFI)
  KREXO_MEM_ACPI_RECLAIM,  // ACPI tables (reclaimable)
  KREXO_MEM_ACPI_NVS,      // ACPI NVS memory
  KREXO_MEM_UNUSABLE,      // Bad memory
  KREXO_MEM_MMIO,          // Memory-mapped I/O
  KREXO_MEM_PERSISTENT,    // Persistent memory (NVDIMM)
} krexo_mem_type_t;

// Unified memory region entry
typedef struct {
  uint64_t base;           // Physical base address
  uint64_t length;         // Length in bytes
  krexo_mem_type_t type;   // Memory type
  uint64_t flags;          // Additional attributes
} krexo_mmap_entry_t;

// Memory map descriptor
typedef struct {
  krexo_mmap_entry_t *entries;
  size_t count;
  size_t capacity;
} krexo_mmap_t;

// Memory attributes flags
#define KREXO_MEM_FLAG_NONE       0
#define KREXO_MEM_FLAG_EXECUTABLE (1 << 0)
#define KREXO_MEM_FLAG_READONLY   (1 << 1)
#define KREXO_MEM_FLAG_NONVOLATILE (1 << 2)

// Platform-specific initialization
// Must be called before using the memory map
int krexo_mmap_init(krexo_mmap_t *mmap);

// Free memory map resources
void krexo_mmap_free(krexo_mmap_t *mmap);

// Get total usable memory (conventional + boot data)
uint64_t krexo_mmap_total_usable(const krexo_mmap_t *mmap);

// Get total conventional (free) memory
uint64_t krexo_mmap_total_free(const krexo_mmap_t *mmap);

// Print memory map (for debugging)
void krexo_mmap_print(const krexo_mmap_t *mmap);

#endif // KREXO_MMAP_H
