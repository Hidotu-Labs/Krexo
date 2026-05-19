#include <common/fb.h>
#include <stdint.h>
#include <stddef.h>

void krexo_fb_draw_image_rect(krexo_fb_t *fb, uint8_t *data, uint32_t x_start, uint32_t y_start, uint32_t w, uint32_t h) {
    if (!data) return;

    // Detect format
    if (data[0] == 'B' && data[1] == 'M') {
        krexo_fb_draw_bmp_rect(fb, data, x_start, y_start, w, h);
    } else if (data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G') {
        krexo_fb_draw_png_rect(fb, data, x_start, y_start, w, h);
    }
}

void krexo_fb_draw_image(krexo_fb_t *fb, uint8_t *data) {
    krexo_fb_draw_image_rect(fb, data, 0, 0, fb->width, fb->height);
}
