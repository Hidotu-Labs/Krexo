#include <Uefi.h>
#include <common/kprint.h>
#include <common/mmap.h>
#include <common/fb.h>

void uefi_init_debug(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);

EFI_STATUS
EFIAPI
efi_main(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
  uefi_init_debug(ImageHandle, SystemTable);

  kprintf("Welcome to Krexo UEFI!\n");
  kprintf("Platform: %s\n", (char *)"x86_64");
  kprintf("SystemTable at: %p\n", SystemTable);
  kprintf("This is a common printf working in UEFI!\n");

  // Initialize and display memory map
  krexo_mmap_t mmap;
  if (krexo_mmap_init(&mmap) == 0) {
    krexo_mmap_print(&mmap);
  } else {
    kprintf("Failed to get memory map!\n");
  }

  // Initialize framebuffer via GOP
  krexo_fb_t fb;
  if (krexo_fb_init(&fb) == 0) {
    krexo_fb_print_info(&fb);
    
    // Draw test rectangles
    kprintf("\nDrawing test pattern...\n");
    krexo_fb_fill_rect(&fb, 10, 10, 100, 100, 0xFF0000FF);   // Blue square
    krexo_fb_fill_rect(&fb, 120, 10, 100, 100, 0xFF00FF00);  // Green square
    krexo_fb_fill_rect(&fb, 230, 10, 100, 100, 0xFFFF0000);  // Red square
    kprintf("Test pattern drawn!\n");
  } else {
    kprintf("Failed to initialize framebuffer!\n");
  }

  kprintf("\nPress any key to continue...");

  UINTN Index;
  EFI_INPUT_KEY Key;
  SystemTable->ConIn->Reset(SystemTable->ConIn, FALSE);
  SystemTable->BootServices->WaitForEvent(1, &SystemTable->ConIn->WaitForKey,
                                          &Index);
  SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key);

  kprintf("\nKey pressed! Exiting...\n");

  while (1)
    ;
  return EFI_SUCCESS;
}
