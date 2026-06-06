#include <common/acpi.h>
#include <common/fb.h>
#include <common/kprint.h>
#include <common/requests.h>
#include <stddef.h>

// A simple bump allocator for responses in a safe region
// We'll put responses at 0x80000 (Safe area below 1MB/Kernel)
static uint64_t response_ptr = 0x80000;

static void *allocate_response(size_t size) {
  // Align to 8 bytes
  response_ptr = (response_ptr + 7) & ~7ULL;
  void *ptr = (void *)(uintptr_t)response_ptr;
  response_ptr += size;
  return ptr;
}

// Forward declaration for SMP discovery
static uint32_t smp_discover_cpus(krexo_smp_info_t **out_cpus, uint32_t *bsp_id);

void requests_handle(void *kernel_start, size_t kernel_size, krexo_fb_t *fb,
                     krexo_mmap_t *mmap, const char *cmdline) {
  uint64_t *ptr = (uint64_t *)kernel_start;
  size_t count = kernel_size / 8;

  uint64_t start_marker[] = KREXO_REQUESTS_START_MARKER;
  uint64_t end_marker[] = KREXO_REQUESTS_END_MARKER;

  size_t start_index = 0;
  size_t end_index = 0;

  // 1. Find the markers
  for (size_t i = 0; i < count - 1; i++) {
    if (ptr[i] == start_marker[0] && ptr[i + 1] == start_marker[1]) {
      start_index = i + 2; // Jump past the marker
    }
    if (ptr[i] == end_marker[0] && ptr[i + 1] == end_marker[1]) {
      end_index = i;
      break;
    }
  }

  // Fallback if markers are missing
  if (start_index == 0 || end_index == 0 || start_index >= end_index) {
    kprintf("[BOOT] Warning: Request markers NOT found. Falling back to full "
            "scan.\n");
    start_index = 0;
    end_index = count - 3;
  } else {
    kprintf("[BOOT] Request markers found! Scanning range: 0x%x - 0x%x\n",
            start_index * 8, end_index * 8);
  }

  // 2. Scan for requests
  for (size_t i = start_index; i < end_index; i++) {
    if (ptr[i] == KREXO_REQUEST_MAGIC_0 &&
        ptr[i + 1] == KREXO_REQUEST_MAGIC_1) {
      uint64_t id_0 = ptr[i + 2];
      uint64_t id_1 = ptr[i + 3];

      // Framebuffer Request
      uint64_t fb_id[] = KREXO_FB_REQUEST_ID;
      if (id_0 == fb_id[0] && id_1 == fb_id[1]) {
        krexo_fb_response_t *resp =
            allocate_response(sizeof(krexo_fb_response_t));
        resp->address = fb->base;
        resp->width = fb->width;
        resp->height = fb->height;
        resp->pitch = fb->pitch;
        resp->bpp = fb->bpp;
        ptr[i + 4] = (uint64_t)(uintptr_t)resp;
        kprintf("[BOOT] Framebuffer request fulfilled: %dx%d\n", fb->width,
                fb->height);
      }

      // Memory Map Request
      uint64_t mmap_id[] = KREXO_MMAP_REQUEST_ID;
      if (id_0 == mmap_id[0] && id_1 == mmap_id[1]) {
        krexo_mmap_response_t *resp =
            allocate_response(sizeof(krexo_mmap_response_t));
        resp->entry_count = mmap->count;
        resp->entries = mmap->entries;
        ptr[i + 4] = (uint64_t)(uintptr_t)resp;
        kprintf("[BOOT] Memory map request fulfilled: %d entries\n",
                (int)mmap->count);
      }

      // Kernel File Request
      uint64_t kfile_id[] = KREXO_KERNEL_FILE_REQUEST_ID;
      if (id_0 == kfile_id[0] && id_1 == kfile_id[1]) {
        krexo_kernel_file_response_t *resp =
            allocate_response(sizeof(krexo_kernel_file_response_t));
        size_t cmd_len = 0;
        while (cmdline[cmd_len])
          cmd_len++;
        char *cmd_buf = allocate_response(cmd_len + 1);
        for (size_t j = 0; j < cmd_len; j++)
          cmd_buf[j] = cmdline[j];
        cmd_buf[cmd_len] = 0;
        resp->cmdline = (uint64_t)(uintptr_t)cmd_buf;
        ptr[i + 4] = (uint64_t)(uintptr_t)resp;
        kprintf("[BOOT] Kernel file/cmdline request fulfilled: '%s'\n",
                cmdline);
      }

      // HHDM Request
      uint64_t hhdm_id[] = KREXO_HHDM_REQUEST_ID;
      if (id_0 == hhdm_id[0] && id_1 == hhdm_id[1]) {
        krexo_hhdm_response_t *resp =
            allocate_response(sizeof(krexo_hhdm_response_t));
        resp->offset = KREXO_HHDM_OFFSET;
        ptr[i + 4] = (uint64_t)(uintptr_t)resp;
        kprintf("[BOOT] HHDM request fulfilled: offset=0x%x%x\n",
                (uint32_t)(KREXO_HHDM_OFFSET >> 32),
                (uint32_t)KREXO_HHDM_OFFSET);
      }

      // SMP Request
      uint64_t smp_id[] = KREXO_SMP_REQUEST_ID;
      if (id_0 == smp_id[0] && id_1 == smp_id[1]) {
        krexo_smp_response_t *resp =
            allocate_response(sizeof(krexo_smp_response_t));
        krexo_smp_info_t *cpus = NULL;
        uint32_t bsp_id = 0;
        uint32_t cpu_count = smp_discover_cpus(&cpus, &bsp_id);

        resp->cpu_count = cpu_count;
        resp->bsp_lapic_id = bsp_id;
        resp->cpus = cpus;
        ptr[i + 4] = (uint64_t)(uintptr_t)resp;
        kprintf("[BOOT] SMP request fulfilled: %d CPUs found (BSP ID: %d)\n",
                (int)cpu_count, (int)bsp_id);
      }
    }
  }
}

// Implementation of SMP discovery using ACPI MADT
static uint32_t smp_discover_cpus(krexo_smp_info_t **out_cpus,
                                  uint32_t *bsp_id) {
  acpi_madt_t *madt = (acpi_madt_t *)acpi_find_table("APIC");
  if (!madt) {
    kprintf("[BOOT] Error: MADT table not found!\n");
    return 0;
  }

  // First pass: count CPUs
  uint32_t count = 0;
  uintptr_t ptr = (uintptr_t)madt + sizeof(acpi_madt_t);
  uintptr_t end = (uintptr_t)madt + madt->header.length;

  while (ptr < end) {
    acpi_madt_entry_t *entry = (acpi_madt_entry_t *)ptr;
    if (entry->type == 0) { // Processor Local APIC
      acpi_madt_lapic_t *lapic = (acpi_madt_lapic_t *)entry;
      if (lapic->flags & 1) { // Enabled
        count++;
      }
    }
    ptr += entry->length;
  }

  if (count == 0)
    return 0;

  krexo_smp_info_t *cpus = allocate_response(sizeof(krexo_smp_info_t) * count);

  // Second pass: fill info
  uint32_t current = 0;
  ptr = (uintptr_t)madt + sizeof(acpi_madt_t);
  while (ptr < end && current < count) {
    acpi_madt_entry_t *entry = (acpi_madt_entry_t *)ptr;
    if (entry->type == 0) {
      acpi_madt_lapic_t *lapic = (acpi_madt_lapic_t *)entry;
      if (lapic->flags & 1) {
        cpus[current].processor_id = lapic->processor_id;
        cpus[current].lapic_id = lapic->lapic_id;
        cpus[current].goto_address = 0;
        cpus[current].extra_argument = 0;
        current++;
      }
    }
    ptr += entry->length;
  }

  // Get current Local APIC ID (simplification)
  uint32_t bsp_lapic = 0;
#if defined(__x86_64__)
  uint32_t ebx;
  __asm__ volatile("cpuid" : "=b"(ebx) : "a"(1) : "ecx", "edx");
  bsp_lapic = (ebx >> 24) & 0xFF;
#endif

  *out_cpus = cpus;
  *bsp_id = bsp_lapic;
  return count;
}
