#include <common/fb.h>
#include <common/font.h>
#include <common/kprint.h>
#include <stdint.h>

void krexo_fb_put_pixel(krexo_fb_t *fb, uint32_t x, uint32_t y, uint32_t color) {
  if (x >= fb->width || y >= fb->height) return;

  if (fb->bpp == 32) {
    uint32_t *pixel = (uint32_t *)((uint8_t *)fb->base + y * fb->pitch + x * 4);

    if (fb->format == KREXO_FB_FORMAT_BGRX_8888) {
      uint8_t r = (color >> 16) & 0xFF;
      uint8_t b = color & 0xFF;
      color = (color & 0xFF00FF00) | (b << 16) | r;
    }

    *pixel = color;
  } else if (fb->bpp == 24) {
    uint8_t *pixel = (uint8_t *)fb->base + y * fb->pitch + x * 3;
    if (fb->format == KREXO_FB_FORMAT_BGRX_8888) {
      pixel[0] = color & 0xFF;           // B
      pixel[1] = (color >> 8) & 0xFF;    // G
      pixel[2] = (color >> 16) & 0xFF;   // R
    } else {
      pixel[0] = (color >> 16) & 0xFF;   // R
      pixel[1] = (color >> 8) & 0xFF;    // G
      pixel[2] = color & 0xFF;           // B
    }
  }
}

void krexo_fb_fill_rect(krexo_fb_t *fb, uint32_t x, uint32_t y,
                        uint32_t w, uint32_t h, uint32_t color) {
  for (uint32_t row = y; row < y + h && row < fb->height; row++) {
    for (uint32_t col = x; col < x + w && col < fb->width; col++) {
      krexo_fb_put_pixel(fb, col, row, color);
    }
  }
}

void krexo_fb_draw_char(krexo_fb_t *fb, uint32_t x, uint32_t y, char c, uint32_t color) {
    if ((unsigned char)c >= 128) return;
    
    for (int row = 0; row < 16; row++) {
        uint8_t row_data = krexo_font8x16[(uint8_t)c][row];
        for (int col = 0; col < 8; col++) {
            if (row_data & (0x80 >> col)) {
                krexo_fb_put_pixel(fb, x + col, y + row, color);
            }
        }
    }
}

void krexo_fb_draw_string(krexo_fb_t *fb, uint32_t x, uint32_t y, const char *str, uint32_t color) {
    while (*str) {
        krexo_fb_draw_char(fb, x, y, *str, color);
        x += 8;
        str++;
    }
}

void krexo_fb_print_info(krexo_fb_t *fb) {
  kprintf("Framebuffer Info:\n");
  kprintf("  Base: %p\n", fb->base);
  kprintf("  Size: %u bytes\n", (uint32_t)fb->size);
  kprintf("  Resolution: %ux%u\n", fb->width, fb->height);
  kprintf("  Pitch: %u bytes\n", fb->pitch);
  kprintf("  BPP: %u\n", fb->bpp);

  const char *fmt_str;
  switch (fb->format) {
    case KREXO_FB_FORMAT_RGBX_8888: fmt_str = "RGBX8888"; break;
    case KREXO_FB_FORMAT_BGRX_8888: fmt_str = "BGRX8888"; break;
    default: fmt_str = "Unknown"; break;
  }
  kprintf("  Format: %s\n", fmt_str);
}
