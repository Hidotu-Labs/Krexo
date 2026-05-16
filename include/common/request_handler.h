#ifndef KREXO_REQUEST_HANDLER_H
#define KREXO_REQUEST_HANDLER_H

#include <common/fb.h>
#include <common/mmap.h>
#include <common/requests.h>
#include <stddef.h>

void requests_handle(void *kernel_start, size_t kernel_size, krexo_fb_t *fb,
                     krexo_mmap_t *mmap, const char *cmdline);

#endif
