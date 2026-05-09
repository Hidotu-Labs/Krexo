#ifndef KREXO_REQUEST_HANDLER_H
#define KREXO_REQUEST_HANDLER_H

#include <common/requests.h>
#include <common/fb.h>
#include <stddef.h>

void requests_handle(void* kernel_start, size_t kernel_size, krexo_fb_t* fb);

#endif
