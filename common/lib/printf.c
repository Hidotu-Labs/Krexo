#include <common/kprint.h>
#include <stdarg.h>
#include <stddef.h>

static char *ks_out = NULL;

static void ksprintf_putc(char c) {
    if (ks_out) {
        *ks_out++ = c;
        *ks_out = '\0';
    }
}

// Simple internal putc dispatcher
static void (*current_putc)(char) = debug_putc;

void kvprintf(const char* format, va_list args) {
    const char* p;
    for (p = format; *p != '\0'; p++) {
        if (*p != '%') {
            current_putc(*p);
            continue;
        }
        p++;
        switch (*p) {
            case 'c': {
                char c = (char)va_arg(args, int);
                current_putc(c);
                break;
            }
            case 's': {
                const char* s = va_arg(args, const char*);
                while (*s) current_putc(*s++);
                break;
            }
            case 'd': {
                int n = va_arg(args, int);
                if (n < 0) {
                    current_putc('-');
                    n = -n;
                }
                char buf[32];
                int i = 0;
                do {
                    buf[i++] = (n % 10) + '0';
                    n /= 10;
                } while (n);
                while (i > 0) current_putc(buf[--i]);
                break;
            }
            case 'X':
            case 'x': {
                unsigned int n = va_arg(args, unsigned int);
                char buf[32];
                int i = 0;
                do {
                    int d = n % 16;
                    buf[i++] = (d < 10) ? (d + '0') : (d - 10 + 'a');
                    n /= 16;
                } while (n);
                while (i > 0) current_putc(buf[--i]);
                break;
            }
            case '%':
                current_putc('%');
                break;
        }
    }
}

void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    current_putc = debug_putc;
    kvprintf(format, args);
    va_end(args);
}

void ksprintf(char* out, const char* format, ...) {
    va_list args;
    va_start(args, format);
    ks_out = out;
    current_putc = ksprintf_putc;
    kvprintf(format, args);
    current_putc = debug_putc;
    va_end(args);
}
