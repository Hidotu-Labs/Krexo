#include <common/kprint.h>
#include <stddef.h>
#include <stdint.h>

static void print_string(const char* s) {
    while (*s) {
        debug_putc(*s++);
    }
}

static void print_uint(uintptr_t n, int base) {
    char buf[64];
    int i = 0;
    const char* digits = "0123456789abcdef";

    if (n == 0) {
        debug_putc('0');
        return;
    }

    while (n > 0) {
        buf[i++] = digits[n % base];
        n /= base;
    }

    while (i > 0) {
        debug_putc(buf[--i]);
    }
}

// Print 64-bit value as hex (no division needed)
static void print_uint64_hex(uint64_t n) {
    char buf[16];
    const char* digits = "0123456789abcdef";
    
    for (int i = 15; i >= 0; i--) {
        buf[i] = digits[n & 0xF];
        n >>= 4;
    }
    
    for (int i = 0; i < 16; i++) {
        debug_putc(buf[i]);
    }
}

static void print_int(intptr_t n) {
    if (n < 0) {
        debug_putc('-');
        n = -n;
    }
    print_uint((uintptr_t)n, 10);
}

void kvprintf(const char* format, va_list args) {
    for (const char* p = format; *p != '\0'; p++) {
        if (*p != '%') {
            debug_putc(*p);
            continue;
        }

        p++;
        
        // Skip width specifier (e.g., "016" in "%016llx")
        while (*p >= '0' && *p <= '9') {
            p++;
        }
        
        // Check for length modifiers
        int is_long = 0;
        int is_longlong = 0;
        
        if (*p == 'l') {
            p++;
            is_long = 1;
            if (*p == 'l') {
                p++;
                is_long = 0;
                is_longlong = 1;
            }
        }
        
        switch (*p) {
            case 's':
                print_string(va_arg(args, char*));
                break;
            case 'd':
                if (is_longlong) {
                    // %lld - not commonly needed in bootloader
                    print_int(va_arg(args, int));
                } else if (is_long) {
                    print_int(va_arg(args, long));
                } else {
                    print_int(va_arg(args, int));
                }
                break;
            case 'u':
                if (is_longlong) {
                    // %llu - print as hex to avoid 64-bit division
                    print_uint64_hex(va_arg(args, uint64_t));
                } else if (is_long) {
                    print_uint(va_arg(args, unsigned long), 10);
                } else {
                    print_uint(va_arg(args, unsigned int), 10);
                }
                break;
            case 'x':
                if (is_longlong) {
                    print_uint64_hex(va_arg(args, uint64_t));
                } else if (is_long) {
                    print_uint(va_arg(args, unsigned long), 16);
                } else {
                    print_uint(va_arg(args, unsigned int), 16);
                }
                break;
            case 'p':
                print_string("0x");
                print_uint(va_arg(args, uintptr_t), 16);
                break;
            case '%':
                debug_putc('%');
                break;
            default:
                debug_putc('%');
                debug_putc(*p);
                break;
        }
    }
}

void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    kvprintf(format, args);
    va_end(args);
}
