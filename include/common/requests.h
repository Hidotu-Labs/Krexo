#ifndef KREXO_REQUESTS_H
#define KREXO_REQUESTS_H

#include <common/mmap.h>
#include <stdint.h>

#define KREXO_REQUEST_MAGIC_0 0x2474fa8644a44516
#define KREXO_REQUEST_MAGIC_1 0x2474fa8644a44517

#define KREXO_REQUESTS_START_MARKER {0x3a3c267b21e84a22, 0xa5e2c140d3f6b981}
#define KREXO_REQUESTS_END_MARKER {0x5bdae12f86c2d49a, 0x9c4f7e21a30d5b68}

// Common header for all requests
typedef struct {
  uint64_t magic[2]; // MAGIC_0, MAGIC_1
  uint64_t id[2];
  uint64_t response;
} krexo_request_header_t;

// --- Framebuffer Request ---
#define KREXO_FB_REQUEST_ID {0x619f72767073286f, 0xc1d5f29d2f21876b}

typedef struct {
  uint64_t address;
  uint32_t width;
  uint32_t height;
  uint32_t pitch;
  uint32_t bpp;
} krexo_fb_response_t;

typedef struct {
  krexo_request_header_t header;
} krexo_fb_request_t;

// --- Memory Map Request ---
#define KREXO_MMAP_REQUEST_ID {0x619f72767073286f, 0xc1d5f29d2f21876c}

typedef struct {
  uint64_t entry_count;
  krexo_mmap_entry_t *entries; // Definitions from mmap.h
} krexo_mmap_response_t;

typedef struct {
  krexo_request_header_t header;
} krexo_mmap_request_t;

// --- Kernel File Request (Required for CMDLINE) ---
#define KREXO_KERNEL_FILE_REQUEST_ID {0x619f72767073286f, 0xc1d5f29d2f21876d}

typedef struct {
  uint64_t
      cmdline; // Virtual address of the null-terminated command line string
} krexo_kernel_file_response_t;

typedef struct {
  krexo_request_header_t header;
} krexo_kernel_file_request_t;

// --- HHDM (Higher Half Direct Map) Request ---
#define KREXO_HHDM_REQUEST_ID {0x619f72767073286f, 0xc1d5f29d2f21876e}

// Default HHDM offset: 0xffff800000000000
// Physical address P is mapped at virtual address (HHDM_OFFSET + P)
#define KREXO_HHDM_OFFSET 0xffff800000000000ULL

typedef struct {
  uint64_t offset; // The HHDM virtual offset
} krexo_hhdm_response_t;

typedef struct {
  krexo_request_header_t header;
} krexo_hhdm_request_t;

// --- SMP (Symmetric Multiprocessing) Request ---
#define KREXO_SMP_REQUEST_ID {0x619f72767073286f, 0xc1d5f29d2f21876f}

typedef struct {
  uint32_t processor_id;
  uint32_t lapic_id;
  uint64_t reserved;
  uint64_t goto_address; // Kernel sets this; AP jumps here
  uint64_t extra_argument;
} krexo_smp_info_t;

typedef struct {
  uint64_t cpu_count;
  uint32_t bsp_lapic_id;
  krexo_smp_info_t *cpus;
} krexo_smp_response_t;

typedef struct {
  krexo_request_header_t header;
  uint64_t flags;
} krexo_smp_request_t;

#endif // KREXO_REQUESTS_H
