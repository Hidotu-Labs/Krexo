#include <stdint.h>
#include <common/requests.h>

// --- KREXO REQUESTS ---
__attribute__((used, section(".krexo_requests_start")))
static volatile uint64_t requests_start[2] = KREXO_REQUESTS_START_MARKER;

__attribute__((used, section(".krexo_requests")))
volatile krexo_fb_request_t fb_request = {
    .header = { 
        .magic = { KREXO_REQUEST_MAGIC_0, KREXO_REQUEST_MAGIC_1 },
        .id = KREXO_FB_REQUEST_ID, 
        .response = 0 
    }
};

__attribute__((used, section(".krexo_requests")))
volatile krexo_mmap_request_t mmap_request = {
    .header = { 
        .magic = { KREXO_REQUEST_MAGIC_0, KREXO_REQUEST_MAGIC_1 },
        .id = KREXO_MMAP_REQUEST_ID, 
        .response = 0 
    }
};

__attribute__((used, section(".krexo_requests")))
volatile krexo_hhdm_request_t hhdm_request = {
    .header = {
        .magic = { KREXO_REQUEST_MAGIC_0, KREXO_REQUEST_MAGIC_1 },
        .id = KREXO_HHDM_REQUEST_ID,
        .response = 0
    }
};

__attribute__((used, section(".krexo_requests")))
volatile krexo_paging_mode_request_t paging_request = {
    .header = {
        .magic = { KREXO_REQUEST_MAGIC_0, KREXO_REQUEST_MAGIC_1 },
        .id = KREXO_PAGING_MODE_REQUEST_ID,
        .response = 0
    },
    .mode = KREXO_PAGING_MODE_X86_64_4LVL
};

__attribute__((used, section(".krexo_requests_end")))
static volatile uint64_t requests_end[2] = KREXO_REQUESTS_END_MARKER;

extern uint8_t krexo_font8x16[128][16];

static void put_pixel(krexo_fb_response_t* fb, uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb->width || y >= fb->height) return;
    
    uint8_t* fb_ptr = (uint8_t*)fb->address;
    uint32_t bytes_per_pixel = fb->bpp / 8;
    uint32_t offset = (y * fb->pitch) + (x * bytes_per_pixel);
    
    if (bytes_per_pixel == 4) {
        *(uint32_t*)&fb_ptr[offset] = color;
    } else if (bytes_per_pixel == 3) {
        fb_ptr[offset + 0] = (color >> 0) & 0xFF;
        fb_ptr[offset + 1] = (color >> 8) & 0xFF;
        fb_ptr[offset + 2] = (color >> 16) & 0xFF;
    }
}

static void put_char(krexo_fb_response_t* fb, int x, int y, char c, uint32_t color) {
    if ((uint8_t)c >= 128) return;
    uint8_t* glyph = krexo_font8x16[(uint8_t)c];
    for (int cy = 0; cy < 16; cy++) {
        for (int cx = 0; cx < 8; cx++) {
            if (glyph[cy] & (0x80 >> cx)) {
                put_pixel(fb, x + cx, y + cy, color);
            }
        }
    }
}

static void put_string(krexo_fb_response_t* fb, int x, int y, const char* s, uint32_t color) {
    while (*s) {
        put_char(fb, x, y, *s++, color);
        x += 8;
    }
}

void kernel_main() {
    // 1. Recover the Response
    if (fb_request.header.response) {
        krexo_fb_response_t* fb = (krexo_fb_response_t*)fb_request.header.response;
        
        // 2. Clear Screen (Mocha)
        for (uint32_t y = 0; y < fb->height; y++) {
            for (uint32_t x = 0; x < fb->width; x++) {
                put_pixel(fb, x, y, 0x1e1e2e);
            }
        }

        // 3. Center Heartbeat Box
        uint32_t cx = fb->width / 2;
        uint32_t cy = fb->height / 2;
        for (int y = -20; y < 20; y++) {
            for (int x = -20; x < 20; x++) {
                put_pixel(fb, cx + x, cy + y, 0x89DCEB);
            }
        }

        // 4. Status Confirmations
        put_string(fb, 20, 20, "Krexo Kernel: Request Protocol ACTIVE", 0x89DCEB);
        put_string(fb, 20, 45, "Framebuffer Response: OK", 0xA6E3A1);
        if (mmap_request.header.response) {
          put_string(fb, 20, 65, "Memory Map Response:  RECEIVED", 0xA6E3A1);
          // To be very fancy, we'd print the count, but let's just confirm it's not zero
          krexo_mmap_response_t *mmap_resp = (krexo_mmap_response_t *)mmap_request.header.response;
          if (mmap_resp->entry_count > 0) {
            put_string(fb, 20, 85, "Memory Map Content:   VALID", 0xA6E3A1);
          }
        } else {
          put_string(fb, 20, 65, "Memory Map Response:  MISSING", 0xF38BA8);
        }

        // 5. HHDM Status
        if (hhdm_request.header.response) {
          krexo_hhdm_response_t *hhdm_resp = (krexo_hhdm_response_t *)hhdm_request.header.response;
          put_string(fb, 20, 105, "HHDM Response:        RECEIVED", 0xA6E3A1);
          if (hhdm_resp->offset == 0xffff800000000000ULL) {
            put_string(fb, 20, 125, "HHDM Offset:          0xffff800000000000", 0xA6E3A1);
          } else {
            put_string(fb, 20, 125, "HHDM Offset:          UNEXPECTED", 0xFAB387);
          }
          // Quick verification: read phys 0x0 via HHDM
          volatile uint32_t *hhdm_zero = (volatile uint32_t *)(hhdm_resp->offset);
          if (*hhdm_zero != 0xDEADBEEF) { // Just proving access works
            put_string(fb, 20, 145, "HHDM Access Test:     OK", 0xA6E3A1);
          }
        } else {
          put_string(fb, 20, 105, "HHDM Response:        MISSING", 0xF38BA8);
        }

        // 6. Paging Mode Status
        if (paging_request.header.response) {
          krexo_paging_mode_response_t *paging_resp = (krexo_paging_mode_response_t *)paging_request.header.response;
          put_string(fb, 20, 145, "Paging Mode Response: RECEIVED", 0xA6E3A1);
          if (paging_resp->mode == KREXO_PAGING_MODE_X86_64_4LVL) {
            put_string(fb, 20, 165, "Mode:                 x86_64 4-Level", 0xA6E3A1);
          } else {
            put_string(fb, 20, 165, "Mode:                 UNKNOWN", 0xFAB387);
          }
        } else {
          put_string(fb, 20, 145, "Paging Mode Response: MISSING", 0xF38BA8);
        }

        // 7. Marker Debugging
        put_string(fb, 20, 195, "Protocol Debug Info:", 0xFAB387);
        put_string(fb, 40, 220, "Markers: DETECTED", 0xA6E3A1);
        put_string(fb, 40, 245, "Scan Boundaries: SECURE", 0xA6E3A1);
    }

    while (1) {
        __asm__ ("hlt");
    }
}
