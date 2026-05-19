#include <common/fb.h>
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
  uint16_t type;
  uint32_t size;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t offset;
} __attribute__((packed)) bmp_file_header_t;

typedef struct {
  uint32_t header_size;
  uint32_t width;
  uint32_t height;
  uint16_t planes;
  uint16_t bpp;
  uint32_t compression;
  uint32_t image_size;
  int32_t x_ppm;
  int32_t y_ppm;
  uint32_t colors_used;
  uint32_t important_colors;
} __attribute__((packed)) bmp_info_header_t;
#pragma pack(pop)

void krexo_fb_draw_bmp_rect(krexo_fb_t *fb, uint8_t *data, uint32_t x_start, uint32_t y_start, uint32_t w, uint32_t h) {
  bmp_file_header_t *file_header = (bmp_file_header_t *)data;
  bmp_info_header_t *info_header = (bmp_info_header_t *)(data + sizeof(bmp_file_header_t));

  if (file_header->type != 0x4D42) return;
  if (info_header->bpp != 24 && info_header->bpp != 32) return;

  int bmp_w = (int)info_header->width;
  int bmp_h = (int)info_header->height;
  int is_top_down = (bmp_h < 0);
  if (bmp_h < 0) bmp_h = -bmp_h;

  uint8_t *pixels = data + file_header->offset;
  int bpp = (int)info_header->bpp / 8;
  int row_size = (bmp_w * bpp + 3) & ~3;

  int off_x = ((int)fb->width - bmp_w) / 2;
  int off_y = ((int)fb->height - bmp_h) / 2;

  // Drawing loop with scaling to screen resolution
  for (uint32_t y = y_start; y < y_start + h && y < fb->height; y++) {
    // In UEFI (64-bit), we use 64-bit products to be extra safe against overflow
    // In BIOS (32-bit), we stick to 32-bit to avoid __udivdi3
#if defined(__x86_64__)
    uint32_t src_y = (uint32_t)(((uint64_t)y * bmp_h) / fb->height);
#else
    uint32_t src_y = (y * (uint32_t)bmp_h) / fb->height;
#endif

    int bmp_row = is_top_down ? (int)src_y : (bmp_h - 1 - (int)src_y);
    uint8_t *row_ptr = pixels + (bmp_row * row_size);
    
    for (uint32_t x = x_start; x < x_start + w && x < fb->width; x++) {
#if defined(__x86_64__)
      uint32_t src_x = (uint32_t)(((uint64_t)x * bmp_w) / fb->width);
#else
      uint32_t src_x = (x * (uint32_t)bmp_w) / fb->width;
#endif
      uint8_t *p = row_ptr + (src_x * bpp);
      // Simplify logic: use put_pixel for color format handling to ensure reliability across all modes
      krexo_fb_put_pixel(fb, x, y, (p[2] << 16) | (p[1] << 8) | p[0]);
    }
  }
}

void krexo_fb_draw_bmp(krexo_fb_t *fb, uint8_t *data) {
  krexo_fb_draw_bmp_rect(fb, data, 0, 0, fb->width, fb->height);
}
