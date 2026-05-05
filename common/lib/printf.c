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
        switch (*p) {
            case 's':
                print_string(va_arg(args, char*));
                break;
            case 'd':
                print_int(va_arg(args, int));
                break;
            case 'x':
                print_uint(va_arg(args, unsigned int), 16);
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
