#include <common/kprint.h>
#include <stdint.h>

void debug_putc(char c) {
    static uint16_t* vga_buffer = (uint16_t*)0xB8000;
    static int cursor_x = 0;
    static int cursor_y = 0;

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        const int index = cursor_y * 80 + cursor_x;
        vga_buffer[index] = (uint16_t)c | (uint16_t)0x0700; // Light grey on black
        cursor_x++;
    }

    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
    }
}

void stage2_main() {
    kprintf("Welcome to Krexo BIOS Stage 2 (Protected Mode)!\n");
    kprintf("We are now running C code on BIOS.\n");
    kprintf("Memory address of stage2_main: %p\n", stage2_main);
    
    while (1);
}
