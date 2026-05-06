#include <Uefi.h>
#include <common/kprint.h>
#include <common/mmap.h>

void uefi_init_debug(EFI_SYSTEM_TABLE *SystemTable);

EFI_STATUS
EFIAPI
efi_main(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
  uefi_init_debug(SystemTable);

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
