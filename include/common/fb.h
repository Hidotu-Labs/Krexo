#pragma once
#include <stdint.h>

typedef enum {
    KREXO_FB_FORMAT_INVALID = 0,
    KREXO_FB_FORMAT_RGBX_8888,
    KREXO_FB_FORMAT_BGRX_8888,
} krexo_fb_format_t;

typedef struct {
    uint64_t base;      // Forced 64-bit for cross-arch handoff
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t format;
    uint32_t bpp;
    uint64_t size;
} krexo_fb_t;

int krexo_fb_init(krexo_fb_t *fb);
void krexo_fb_fill_rect(krexo_fb_t *fb, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void krexo_fb_draw_string(krexo_fb_t *fb, uint32_t x, uint32_t y, const char *str, uint32_t color);
