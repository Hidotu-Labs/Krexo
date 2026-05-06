#ifndef KREXO_FB_H
#define KREXO_FB_H

#include <stdint.h>

typedef enum {
  KREXO_FB_FORMAT_INVALID = 0,
  KREXO_FB_FORMAT_RGBX_8888,
  KREXO_FB_FORMAT_BGRX_8888,
  KREXO_FB_FORMAT_BITMAP,
} krexo_fb_format_t;

typedef struct {
  void *base;
  uint64_t size;
  uint32_t width;
  uint32_t height;
  uint32_t pitch;      // bytes per scanline
  uint32_t bpp;        // bits per pixel
  krexo_fb_format_t format;
} krexo_fb_t;

// Initialize framebuffer (platform-specific)
int krexo_fb_init(krexo_fb_t *fb);

// Draw a pixel at (x, y) with color
void krexo_fb_put_pixel(krexo_fb_t *fb, uint32_t x, uint32_t y, uint32_t color);

// Fill rectangle
void krexo_fb_fill_rect(krexo_fb_t *fb, uint32_t x, uint32_t y, 
                        uint32_t w, uint32_t h, uint32_t color);

// Print framebuffer info
void krexo_fb_print_info(krexo_fb_t *fb);

#endif // KREXO_FB_H
