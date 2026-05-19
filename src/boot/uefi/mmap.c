#include <Uefi.h>
#include <common/mmap.h>
#include <common/kprint.h>

// Global Boot Services pointer (set by uefi_init_debug)
extern EFI_BOOT_SERVICES *gBS;

// Convert UEFI memory type to Krexo unified type
static krexo_mem_type_t uefi_to_krexo_type(EFI_MEMORY_TYPE uefi_type) {
  switch (uefi_type) {
    case EfiConventionalMemory:
      return KREXO_MEM_CONVENTIONAL;
    case EfiBootServicesCode:
    case EfiBootServicesData:
      return KREXO_MEM_BOOT_DATA;
    case EfiRuntimeServicesCode:
      return KREXO_MEM_RUNTIME_CODE;
    case EfiRuntimeServicesData:
      return KREXO_MEM_RUNTIME_DATA;
    case EfiACPIReclaimMemory:
      return KREXO_MEM_ACPI_RECLAIM;
    case EfiACPIMemoryNVS:
      return KREXO_MEM_ACPI_NVS;
    case EfiUnusableMemory:
      return KREXO_MEM_UNUSABLE;
    case EfiMemoryMappedIO:
    case EfiMemoryMappedIOPortSpace:
      return KREXO_MEM_MMIO;
    case EfiPersistentMemory:
      return KREXO_MEM_PERSISTENT;
    default:
      return KREXO_MEM_RESERVED;
  }
}

// Convert UEFI attributes to Krexo flags
static uint64_t uefi_to_krexo_flags(UINT64 attributes) {
  uint64_t flags = KREXO_MEM_FLAG_NONE;
  
  if (!(attributes & EFI_MEMORY_XP)) {
    flags |= KREXO_MEM_FLAG_EXECUTABLE;
  }
  if (attributes & EFI_MEMORY_RO) {
    flags |= KREXO_MEM_FLAG_READONLY;
  }
  if (attributes & EFI_MEMORY_NV) {
    flags |= KREXO_MEM_FLAG_NONVOLATILE;
  }
  
  return flags;
}

// Global storage for memory map
static krexo_mmap_entry_t g_entries[128];

int krexo_mmap_init(krexo_mmap_t *mmap) {
  EFI_STATUS status;
  UINTN map_size = 0;
  UINTN map_key;
  UINTN desc_size;
  UINT32 desc_version;
  EFI_MEMORY_DESCRIPTOR *map = NULL;
  
  // Get the memory map size first
  status = gBS->GetMemoryMap(&map_size, map, &map_key, &desc_size, &desc_version);
  if (status != EFI_BUFFER_TOO_SMALL) {
    return -1;
  }
  
  // Allocate buffer for memory map
  // Add extra space for potential changes
  map_size += 2 * desc_size;
  status = gBS->AllocatePool(EfiBootServicesData, map_size, (void **)&map);
  if (EFI_ERROR(status)) {
    return -1;
  }
  
  // Get the actual memory map
  status = gBS->GetMemoryMap(&map_size, map, &map_key, &desc_size, &desc_version);
  if (EFI_ERROR(status)) {
    gBS->FreePool(map);
    return -1;
  }
  
  // Count entries
  UINTN entry_count = map_size / desc_size;
  
  // Limit to our static buffer size
  if (entry_count > 128) {
    entry_count = 128;
  }
  
  // Convert to unified format
  mmap->entries = g_entries;
  mmap->capacity = 128;
  mmap->count = 0;
  
  EFI_MEMORY_DESCRIPTOR *desc = map;
  for (UINTN i = 0; i < entry_count; i++) {
    krexo_mmap_entry_t *entry = &mmap->entries[mmap->count];
    
    entry->base = desc->PhysicalStart;
    entry->length = (uint64_t)desc->NumberOfPages * 4096;
    entry->type = uefi_to_krexo_type(desc->Type);
    entry->flags = uefi_to_krexo_flags(desc->Attribute);
    
    mmap->count++;
    desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)desc + desc_size);
  }
  
  mmap->key = (uint64_t)map_key;
  gBS->FreePool(map);
  return 0;
}

void krexo_mmap_free(krexo_mmap_t *mmap) {
  // Using static buffer, nothing to free
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
