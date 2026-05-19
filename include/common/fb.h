#pragma once
#include <stdint.h>

typedef enum {
  KREXO_FB_FORMAT_INVALID = 0,
  KREXO_FB_FORMAT_RGBX_8888,
  KREXO_FB_FORMAT_BGRX_8888,
} krexo_fb_format_t;

typedef struct {
  uint64_t base; // Forced 64-bit for cross-arch handoff
  uint32_t width;
  uint32_t height;
  uint32_t pitch;
  uint32_t format;
  uint32_t bpp;
  uint64_t size;
} krexo_fb_t;

int krexo_fb_init(krexo_fb_t *fb);
void krexo_fb_put_pixel(krexo_fb_t *fb, uint32_t x, uint32_t y, uint32_t color);
uint32_t krexo_fb_get_pixel(krexo_fb_t *fb, uint32_t x, uint32_t y);
void krexo_fb_fill_rect(krexo_fb_t *fb, uint32_t x, uint32_t y, uint32_t w,
                        uint32_t h, uint32_t color);
void krexo_fb_draw_rect_outline(krexo_fb_t *fb, uint32_t x, uint32_t y,
                                uint32_t w, uint32_t h, uint32_t color);
void krexo_fb_draw_rect_transparent(krexo_fb_t *fb, uint32_t x, uint32_t y,
                                    uint32_t w, uint32_t h, uint32_t color,
                                    uint8_t opacity);
void krexo_fb_draw_noise_bg_rect(krexo_fb_t *fb, uint32_t x_start, uint32_t y_start, uint32_t w, uint32_t h, uint32_t base_color);
void krexo_fb_draw_noise_bg(krexo_fb_t *fb, uint32_t base_color);
void krexo_fb_draw_gradient(krexo_fb_t *fb, uint32_t color_top,
                            uint32_t color_bottom);
void krexo_fb_draw_string(krexo_fb_t *fb, uint32_t x, uint32_t y,
                          const char *str, uint32_t color);
void krexo_fb_draw_string_shadow(krexo_fb_t *fb, uint32_t x, uint32_t y,
                                 const char *str, uint32_t color,
                                 uint32_t shadow_color);
void krexo_fb_draw_bmp_rect(krexo_fb_t *fb, uint8_t *data, uint32_t x_start, uint32_t y_start, uint32_t w, uint32_t h);
void krexo_fb_draw_bmp(krexo_fb_t *fb, uint8_t *data);
void krexo_fb_draw_png_rect(krexo_fb_t *fb, uint8_t *data, uint32_t x_start, uint32_t y_start, uint32_t w, uint32_t h);
void krexo_fb_draw_png(krexo_fb_t *fb, uint8_t *data);
void krexo_fb_draw_image_rect(krexo_fb_t *fb, uint8_t *data, uint32_t x_start, uint32_t y_start, uint32_t w, uint32_t h);
void krexo_fb_draw_image(krexo_fb_t *fb, uint8_t *data);
