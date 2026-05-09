#ifndef KREXO_FONT_H
#define KREXO_FONT_H

#include <stdint.h>

// Simple 8x16 bitmapped font (Subset of ASCII 32-126)
// Each character is 16 bytes.
extern const uint8_t krexo_font8x16[128][16];

#endif // KREXO_FONT_H
