#include <common/requests.h>
#include <common/fb.h>
#include <common/kprint.h>
#include <stddef.h>

// A simple bump allocator for responses in a safe region
// We'll put responses at 0x80000 (Safe area below 1MB/Kernel)
static uint64_t response_ptr = 0x80000;

static void* allocate_response(size_t size) {
    void* ptr = (void*)(uintptr_t)response_ptr;
    response_ptr += size;
    return ptr;
}

void requests_handle(void* kernel_start, size_t kernel_size, krexo_fb_t* fb) {
    uint64_t* ptr = (uint64_t*)kernel_start;
    size_t count = kernel_size / 8;

    for (size_t i = 0; i < count - 3; i++) {
        if (ptr[i] == KREXO_REQUEST_MAGIC_0 && ptr[i+1] == KREXO_REQUEST_MAGIC_1) {
            // Found a request! Check ID
            uint64_t id_0 = ptr[i+2];
            uint64_t id_1 = ptr[i+3];

            // Framebuffer Request ID
            uint64_t fb_id[] = KREXO_FB_REQUEST_ID;
            if (id_0 == fb_id[0] && id_1 == fb_id[1]) {
                krexo_fb_response_t* resp = allocate_response(sizeof(krexo_fb_response_t));
                resp->address = fb->base;
                resp->width = fb->width;
                resp->height = fb->height;
                resp->pitch = fb->pitch;
                resp->bpp = fb->bpp;
                
                // Set the response pointer (ptr[i+4] is the response field)
                ptr[i+4] = (uint64_t)(uintptr_t)resp;
            }
            
            // Memory Map Request ID
            uint64_t mmap_id[] = KREXO_MMAP_REQUEST_ID;
            if (id_0 == mmap_id[0] && id_1 == mmap_id[1]) {
                // To be implemented: Pass mmap
                // We need to pass the actual mmap entries here
            }
        }
    }
}
