#include <common/fb.h>
#include <common/font.h>
#include <common/kprint.h>
#include <stdint.h>

uint32_t krexo_fb_get_pixel(krexo_fb_t *fb, uint32_t x, uint32_t y) {
  if (x >= fb->width || y >= fb->height)
    return 0;
  if (fb->bpp == 32) {
    uint32_t color = *(uint32_t *)((uint8_t *)fb->base + y * fb->pitch + x * 4);
    
    // If hardware is BGR, the 32-bit value in memory is 0xXXRRGGBB (Little Endian)
    // No, wait. 
    // If Byte 0=B, 1=G, 2=R, then as a 32-bit uint it is 0xXXRRGGBB.
    // If Byte 0=R, 1=G, 2=B, then as a 32-bit uint it is 0xXXBBGGRR.
    
    if (fb->format == KREXO_FB_FORMAT_RGBX_8888) {
      // Swap R and B to get 0xRRGGBB constant format
      uint8_t r = color & 0xFF;
      uint8_t b = (color >> 16) & 0xFF;
      color = (color & 0xFF00FF00) | (r << 16) | b;
    }
    return color & 0xFFFFFF;
  }
  return 0;
}

void krexo_fb_put_pixel(krexo_fb_t *fb, uint32_t x, uint32_t y,
                        uint32_t color) {
  if (x >= fb->width || y >= fb->height)
    return;

  if (fb->bpp == 32) {
    uint32_t *pixel = (uint32_t *)((uint8_t *)fb->base + y * fb->pitch + x * 4);

    if (fb->format == KREXO_FB_FORMAT_RGBX_8888) {
      uint8_t r = (color >> 16) & 0xFF;
      uint8_t b = color & 0xFF;
      color = (color & 0xFF00FF00) | (b << 16) | r;
    }

    *pixel = color;
  } else if (fb->bpp == 24) {
    uint8_t *pixel = (uint8_t *)fb->base + y * fb->pitch + x * 3;
    if (fb->format == KREXO_FB_FORMAT_BGRX_8888) {
      pixel[0] = color & 0xFF;         // B
      pixel[1] = (color >> 8) & 0xFF;  // G
      pixel[2] = (color >> 16) & 0xFF; // R
    } else {
      pixel[0] = (color >> 16) & 0xFF; // R
      pixel[1] = (color >> 8) & 0xFF;  // G
      pixel[2] = color & 0xFF;         // B
    }
  }
}

void krexo_fb_fill_rect(krexo_fb_t *fb, uint32_t x, uint32_t y, uint32_t w,
                        uint32_t h, uint32_t color) {
  for (uint32_t row = y; row < y + h && row < fb->height; row++) {
    for (uint32_t col = x; col < x + w && col < fb->width; col++) {
      krexo_fb_put_pixel(fb, col, row, color);
    }
  }
}

void krexo_fb_draw_rect_outline(krexo_fb_t *fb, uint32_t x, uint32_t y,
                                uint32_t w, uint32_t h, uint32_t color) {
  for (uint32_t col = x; col < x + w; col++) {
    krexo_fb_put_pixel(fb, col, y, color);
    krexo_fb_put_pixel(fb, col, y + h - 1, color);
  }
  for (uint32_t row = y; row < y + h; row++) {
    krexo_fb_put_pixel(fb, x, row, color);
    krexo_fb_put_pixel(fb, x + w - 1, row, color);
  }
}

void krexo_fb_draw_gradient(krexo_fb_t *fb, uint32_t color_top,
                            uint32_t color_bottom) {
  for (uint32_t y = 0; y < fb->height; y++) {
    uint8_t r1 = (color_top >> 16) & 0xFF;
    uint8_t g1 = (color_top >> 8) & 0xFF;
    uint8_t b1 = color_top & 0xFF;

    uint8_t r2 = (color_bottom >> 16) & 0xFF;
    uint8_t g2 = (color_bottom >> 8) & 0xFF;
    uint8_t b2 = color_bottom & 0xFF;

    uint8_t r = r1 + (int)(r2 - r1) * (int)y / (int)fb->height;
    uint8_t g = g1 + (int)(g2 - g1) * (int)y / (int)fb->height;
    uint8_t b = b1 + (int)(b2 - b1) * (int)y / (int)fb->height;

    uint32_t color = (r << 16) | (g << 8) | b;

    // Optimize: Draw a horizontal line directly if we had a hline function,
    // but for now let's just fill
    for (uint32_t x = 0; x < fb->width; x++) {
      krexo_fb_put_pixel(fb, x, y, color);
    }
  }
}

void krexo_fb_draw_char(krexo_fb_t *fb, uint32_t x, uint32_t y, char c,
                        uint32_t color) {
  if ((unsigned char)c >= 128)
    return;

  for (int row = 0; row < 16; row++) {
    uint8_t row_data = krexo_font8x16[(uint8_t)c][row];
    for (int col = 0; col < 8; col++) {
      if (row_data & (0x80 >> col)) {
        krexo_fb_put_pixel(fb, x + col, y + row, color);
      }
    }
  }
}

void krexo_fb_draw_string(krexo_fb_t *fb, uint32_t x, uint32_t y,
                          const char *str, uint32_t color) {
  while (*str) {
    krexo_fb_draw_char(fb, x, y, *str, color);
    x += 8;
    str++;
  }
}

void krexo_fb_draw_noise_bg(krexo_fb_t *fb, uint32_t base_color) {
  // A simple deterministic pseudo-random background (stars/nebula effect)
  // We use the coordinate to seed the 'noise'
  uint8_t r_base = (base_color >> 16) & 0xFF;
  uint8_t g_base = (base_color >> 8) & 0xFF;
  uint8_t b_base = base_color & 0xFF;

  for (uint32_t y = 0; y < fb->height; y++) {
    for (uint32_t x = 0; x < fb->width; x++) {
      // Simple hash for "stars"
      uint32_t hash = (x * 1597463007U) ^ (y * 2731467445U);
      hash ^= (hash >> 16);
      hash *= 0x85ebca6b;

      uint32_t color = 0;
      if ((hash & 0xFFF) == 0) { // Tiny star
        color = 0xAAAAAA;
      } else if ((hash & 0x7FFF) == 0) { // Bright star
        color = 0xFFFFFF;
      } else {
        // Subtle color variation based on Y (cheap gradient)
        uint8_t r = (r_base * (fb->height - y)) / fb->height;
        uint8_t g = (g_base * (fb->height - y)) / fb->height;
        uint8_t b = (b_base * (fb->height - y)) / fb->height;

        // Add tiny organic noise
        r = (r > (hash & 0x7)) ? r - (hash & 0x7) : 0;
        b = (b < (255 - (hash & 0x3))) ? b + (hash & 0x3) : 255;

        color = (r << 16) | (g << 8) | b;
      }
      krexo_fb_put_pixel(fb, x, y, color);
    }
  }
}

void krexo_fb_draw_rect_transparent(krexo_fb_t *fb, uint32_t x, uint32_t y,
                                    uint32_t w, uint32_t h, uint32_t color,
                                    uint8_t opacity) {
  // Fixed-point blending (approximate)
  uint8_t r_src = (color >> 16) & 0xFF;
  uint8_t g_src = (color >> 8) & 0xFF;
  uint8_t b_src = color & 0xFF;

  for (uint32_t row = y; row < y + h && row < fb->height; row++) {
    for (uint32_t col = x; col < x + w && col < fb->width; col++) {
      uint32_t bg = krexo_fb_get_pixel(fb, col, row);
      uint8_t r_bg = (bg >> 16) & 0xFF;
      uint8_t g_bg = (bg >> 8) & 0xFF;
      uint8_t b_bg = bg & 0xFF;

      uint8_t r = (r_src * opacity + r_bg * (255 - opacity)) >> 8;
      uint8_t g = (g_src * opacity + g_bg * (255 - opacity)) >> 8;
      uint8_t b = (b_src * opacity + b_bg * (255 - opacity)) >> 8;

      krexo_fb_put_pixel(fb, col, row, (r << 16) | (g << 8) | b);
    }
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
  case KREXO_FB_FORMAT_RGBX_8888:
    fmt_str = "RGBX8888";
    break;
  case KREXO_FB_FORMAT_BGRX_8888:
    fmt_str = "BGRX8888";
    break;
  default:
    fmt_str = "Unknown";
    break;
  }
  kprintf("  Format: %s\n", fmt_str);
}
