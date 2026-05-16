#include <common/fb.h>
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
  uint16_t type;
  uint32_t size;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t offset;
} bmp_file_header_t;

typedef struct {
  uint32_t size;
  int32_t width;
  int32_t height;
  uint16_t planes;
  uint16_t bpp;
  uint32_t compression;
  uint32_t image_size;
  int32_t x_ppm;
  int32_t y_ppm;
  uint32_t colors_used;
  uint32_t colors_important;
} bmp_info_header_t;
#pragma pack(pop)

void krexo_fb_draw_bmp(krexo_fb_t *fb, uint8_t *data) {
  bmp_file_header_t *file_header = (bmp_file_header_t *)data;
  bmp_info_header_t *info_header =
      (bmp_info_header_t *)(data + sizeof(bmp_file_header_t));

  if (file_header->type != 0x4D42)
    return; // 'BM'
  if (info_header->bpp != 24 && info_header->bpp != 32)
    return;

  int width = info_header->width;
  int height =
      (info_header->height < 0) ? -info_header->height : info_header->height;
  int is_top_down = (info_header->height < 0);

  uint8_t *pixels = data + file_header->offset;
  int row_size = (width * (info_header->bpp / 8) + 3) & ~3;

  for (int y = 0; y < height && y < fb->height; y++) {
    for (int x = 0; x < width && x < fb->width; x++) {
      int bmp_y = is_top_down ? y : (height - 1 - y);
      uint8_t *p = pixels + (bmp_y * row_size) + (x * (info_header->bpp / 8));

      uint32_t color = (p[2] << 16) | (p[1] << 8) | p[0];
      krexo_fb_put_pixel(fb, x, y, color);
    }
  }
}
