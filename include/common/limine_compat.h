#ifndef KREXO_LIMINE_COMPAT_H
#define KREXO_LIMINE_COMPAT_H

#include <common/fb.h>
#include <common/mmap.h>
#include <stddef.h>
#include <stdint.h>

// Detect if a loaded kernel uses the Limine protocol
// Returns 1 if Limine protocol markers are found, 0 otherwise
int limine_detect(void *kernel_start, size_t kernel_size);

// Handle Limine protocol requests in a loaded kernel
// Scans for Limine-format requests and fills in responses
// If out_entry is non-NULL and an entry point request is found,
// the custom entry point will be written there
void limine_handle_requests(void *kernel_start, size_t kernel_size,
                            krexo_fb_t *fb, krexo_mmap_t *mmap,
                            const char *cmdline, uint64_t kernel_phys_base,
                            uint64_t *out_entry);

#endif // KREXO_LIMINE_COMPAT_H
