#include <common/fb.h>
#include <common/kprint.h>
#include <stdint.h>

/*
 * VBE info is stored by stage1.s before entering protected mode.
 *
 * Layout at 0x6000 (vbe_fb_info):
 *   Offset  Size   Field
 *   0x00    4      Framebuffer physical address (32-bit)
 *   0x04    2      X resolution
 *   0x06    2      Y resolution
 *   0x08    2      Pitch (bytes per scanline)
 *   0x0A    1      BPP (bits per pixel)
 *   0x0B    1      Red field position
 *   0x0C    1      Green field position
 *   0x0D    1      Blue field position
 */
#define VBE_FB_INFO_ADDR  0x6000

typedef struct {
  uint32_t framebuffer;
  uint16_t width;
  uint16_t height;
  uint16_t pitch;
  uint8_t  bpp;
  uint8_t  red_position;
  uint8_t  green_position;
  uint8_t  blue_position;
} __attribute__((packed)) vbe_fb_info_t;

int krexo_fb_init(krexo_fb_t *fb) {
  volatile vbe_fb_info_t *info = (volatile vbe_fb_info_t *)VBE_FB_INFO_ADDR;

  // Validate: stage1 zeroes this region first; if framebuffer is still 0
  // it means VBE setup failed or was skipped.
  if (info->framebuffer == 0 || info->width == 0 || info->height == 0) {
    kprintf("VBE: No framebuffer info found at 0x%x\n", VBE_FB_INFO_ADDR);
    return -1;
  }

  fb->base   = (uint64_t)info->framebuffer;
  fb->width  = info->width;
  fb->height = info->height;
  fb->pitch  = info->pitch;
  fb->bpp    = info->bpp;
  fb->size   = (uint64_t)info->pitch * info->height;

  // Determine pixel format from colour field positions
  if (info->bpp == 32 || info->bpp == 24) {
    if (info->red_position == 0) {
      fb->format = KREXO_FB_FORMAT_RGBX_8888;
    } else {
      fb->format = KREXO_FB_FORMAT_BGRX_8888;
    }
  } else {
    fb->format = KREXO_FB_FORMAT_INVALID;
  }

  return 0;
}

// krexo_fb_put_pixel, krexo_fb_fill_rect, and krexo_fb_print_info 
// are now implemented in common/lib/fb.c
