#include <Uefi.h>
#include <common/kprint.h>

static EFI_SYSTEM_TABLE* gST = NULL;

void uefi_init_debug(EFI_SYSTEM_TABLE* SystemTable) {
    gST = SystemTable;
}

void debug_putc(char c) {
    if (!gST) return;
    
    CHAR16 buf[2];
    buf[0] = (CHAR16)c;
    buf[1] = 0;

    // Handle newline transformation for UEFI
    if (c == '\n') {
        CHAR16 newline[] = {L'\r', L'\n', 0};
        gST->ConOut->OutputString(gST->ConOut, newline);
    } else {
        gST->ConOut->OutputString(gST->ConOut, buf);
    }
}
