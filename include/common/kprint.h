#ifndef KPRINT_H
#define KPRINT_H

#include <stdarg.h>

void kprintf(const char* format, ...);
void ksprintf(char* out, const char* format, ...);
void kvprintf(const char* format, va_list args);

// This is what each platform must implement
void debug_putc(char c);

#endif
