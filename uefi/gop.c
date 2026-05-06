#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <common/fb.h>
#include <common/kprint.h>

// Global Boot Services pointer (set by uefi_init_debug)
extern EFI_BOOT_SERVICES *gBS;
extern EFI_HANDLE gImageHandle;

// Global GOP protocol pointer
static EFI_GRAPHICS_OUTPUT_PROTOCOL *gGop = NULL;

// Define GOP GUID (from MdePkg.dec)
EFI_GUID gEfiGraphicsOutputProtocolGuid = 
  { 0x9042A9DE, 0x23DC, 0x4A38, { 0x96, 0xFB, 0x7A, 0xDE, 0xD0, 0x80, 0x51, 0x6A }};

// Convert EFI pixel format to Krexo format
static krexo_fb_format_t efi_to_krexo_format(EFI_GRAPHICS_PIXEL_FORMAT efi_fmt) {
  switch (efi_fmt) {
    case PixelRedGreenBlueReserved8BitPerColor:
      return KREXO_FB_FORMAT_RGBX_8888;
    case PixelBlueGreenRedReserved8BitPerColor:
      return KREXO_FB_FORMAT_BGRX_8888;
    case PixelBitMask:
    case PixelBltOnly:
    default:
      return KREXO_FB_FORMAT_INVALID;
  }
}

int krexo_fb_init(krexo_fb_t *fb) {
  EFI_STATUS status;
  EFI_HANDLE *handles = NULL;
  UINTN num_handles = 0;

  // Locate GOP handles
  status = gBS->LocateHandleBuffer(
      ByProtocol,
      &gEfiGraphicsOutputProtocolGuid,
      NULL,
      &num_handles,
      &handles);

  if (EFI_ERROR(status) || num_handles == 0) {
    kprintf("GOP: No graphics output protocol found\n");
    return -1;
  }

  // Open GOP protocol on first handle
  status = gBS->OpenProtocol(
      handles[0],
      &gEfiGraphicsOutputProtocolGuid,
      (void **)&gGop,
      gImageHandle,
      NULL,
      EFI_OPEN_PROTOCOL_GET_PROTOCOL);

  gBS->FreePool(handles);

  if (EFI_ERROR(status)) {
    kprintf("GOP: Failed to open graphics protocol\n");
    return -1;
  }

  // Set preferred mode (1024x768 if available, otherwise current)
  UINT32 preferred_mode = gGop->Mode->Mode;
  UINT32 target_width = 1024;
  UINT32 target_height = 768;
  
  for (UINT32 i = 0; i < gGop->Mode->MaxMode; i++) {
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    UINTN info_size;
    
    status = gGop->QueryMode(gGop, i, &info_size, &info);
    if (EFI_ERROR(status)) continue;
    
    if (info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor ||
        info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
      if (info->HorizontalResolution == target_width &&
          info->VerticalResolution == target_height) {
        preferred_mode = i;
        break;
      }
      // Fallback to first valid mode if preferred not found
      if (preferred_mode == gGop->Mode->Mode) {
        preferred_mode = i;
      }
    }
  }

  // Set the mode
  if (preferred_mode != gGop->Mode->Mode) {
    status = gGop->SetMode(gGop, preferred_mode);
    if (EFI_ERROR(status)) {
      kprintf("GOP: Failed to set mode %d\n", preferred_mode);
    }
  }

  // Fill framebuffer info
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode = gGop->Mode;
  fb->base = (void *)mode->FrameBufferBase;
  fb->size = mode->FrameBufferSize;
  fb->width = mode->Info->HorizontalResolution;
  fb->height = mode->Info->VerticalResolution;
  fb->pitch = mode->Info->PixelsPerScanLine * 4;
  fb->bpp = 32;
  fb->format = efi_to_krexo_format(mode->Info->PixelFormat);

  return 0;
}

void krexo_fb_put_pixel(krexo_fb_t *fb, uint32_t x, uint32_t y, uint32_t color) {
  if (x >= fb->width || y >= fb->height) return;
  
  uint32_t *pixel = (uint32_t *)((uint8_t *)fb->base + y * fb->pitch + x * 4);
  
  // Handle format conversion
  if (fb->format == KREXO_FB_FORMAT_BGRX_8888) {
    // Swap R and B
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t b = color & 0xFF;
    color = (color & 0xFF00FF00) | (b << 16) | r;
  }
  
  *pixel = color;
}

void krexo_fb_fill_rect(krexo_fb_t *fb, uint32_t x, uint32_t y, 
                        uint32_t w, uint32_t h, uint32_t color) {
  for (uint32_t row = y; row < y + h && row < fb->height; row++) {
    for (uint32_t col = x; col < x + w && col < fb->width; col++) {
      krexo_fb_put_pixel(fb, col, row, color);
    }
  }
}

void krexo_fb_print_info(krexo_fb_t *fb) {
  kprintf("Framebuffer Info:\n");
  kprintf("  Base: %p\n", fb->base);
  kprintf("  Size: %lu bytes\n", fb->size);
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
