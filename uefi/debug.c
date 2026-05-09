#include <Uefi.h>
#include <common/kprint.h>

static EFI_SYSTEM_TABLE* gST = NULL;
EFI_BOOT_SERVICES *gBS = NULL;
EFI_HANDLE gImageHandle = NULL;

static inline void outb(UINT16 port, UINT8 val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline UINT8 inb(UINT16 port) {
    UINT8 ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static void serial_putc(char c) {
    while ((inb(0x3f8 + 5) & 0x20) == 0);
    outb(0x3f8, (UINT8)c);
}

void uefi_init_debug(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    gImageHandle = ImageHandle;
    gST = SystemTable;
    gBS = SystemTable->BootServices;
    
    // Initialize COM1
    outb(0x3f8 + 1, 0x00);
    outb(0x3f8 + 3, 0x80);
    outb(0x3f8 + 0, 0x03);
    outb(0x3f8 + 1, 0x00);
    outb(0x3f8 + 3, 0x03);
    outb(0x3f8 + 2, 0xC7);
    outb(0x3f8 + 4, 0x0B);
}

void debug_putc(char c) {
    serial_putc(c);
    
    if (!gST) return;
    
    CHAR16 buf[2];
    buf[0] = (CHAR16)c;
    buf[1] = 0;

    if (c == '\n') {
        CHAR16 newline[] = {L'\r', L'\n', 0};
        gST->ConOut->OutputString(gST->ConOut, newline);
    } else {
        gST->ConOut->OutputString(gST->ConOut, buf);
    }
}
