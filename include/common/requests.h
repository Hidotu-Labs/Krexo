#ifndef KREXO_REQUESTS_H
#define KREXO_REQUESTS_H

#include <stdint.h>
#include <common/mmap.h>

#define KREXO_REQUEST_MAGIC_0 0x2474fa8644a44516
#define KREXO_REQUEST_MAGIC_1 0x2474fa8644a44517

// Common header for all requests
typedef struct {
    uint64_t magic[2]; // MAGIC_0, MAGIC_1
    uint64_t id[2];
    uint64_t response;
} krexo_request_header_t;

// --- Framebuffer Request ---
#define KREXO_FB_REQUEST_ID { 0x619f72767073286f, 0xc1d5f29d2f21876b }

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
#define KREXO_MMAP_REQUEST_ID { 0x619f72767073286f, 0xc1d5f29d2f21876c }

typedef struct {
    uint64_t entry_count;
    krexo_mmap_entry_t* entries; // Definitions from mmap.h
} krexo_mmap_response_t;

typedef struct {
    krexo_request_header_t header;
} krexo_mmap_request_t;

#endif // KREXO_REQUESTS_H
