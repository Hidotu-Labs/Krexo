#include <common/fb.h>
#include <stdint.h>
#include <stddef.h>

/* --- Tiny Inflate (tinf) --- */

typedef struct {
    const uint8_t *source;
    uint32_t tag;
    int bitcount;
} tinf_data_t;

static uint32_t tinf_getbits(tinf_data_t *d, int num) {
    uint32_t v = d->tag;
    while (d->bitcount < num) {
        v |= (uint32_t)(*d->source++) << d->bitcount;
        d->bitcount += 8;
    }
    d->tag = v >> num;
    d->bitcount -= num;
    return v & ((1 << num) - 1);
}

typedef struct {
    uint16_t counts[16];
    uint16_t symbols[288];
} tinf_tree_t;

static int tinf_decode_symbol(tinf_data_t *d, const tinf_tree_t *t) {
    int cur = 0, first = 0, index = 0;
    for (int len = 1; len <= 15; len++) {
        cur = (cur << 1) | tinf_getbits(d, 1);
        int count = t->counts[len];
        if (cur < first + count) return t->symbols[index + (cur - first)];
        index += count;
        first = (first + count) << 1;
    }
    return -1;
}

static void tinf_build_tree(tinf_tree_t *t, const uint8_t *lengths, int num) {
    uint16_t offsets[16];
    for (int i = 0; i < 16; i++) t->counts[i] = 0;
    for (int i = 0; i < num; i++) if (lengths[i]) t->counts[lengths[i]]++;
    offsets[1] = 0;
    for (int i = 1; i < 15; i++) offsets[i+1] = offsets[i] + t->counts[i];
    for (int i = 0; i < num; i++) if (lengths[i]) t->symbols[offsets[lengths[i]]++] = i;
}

static const uint8_t tinf_order[] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
static const uint16_t tinf_length_base[] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258 };
static const uint8_t tinf_length_ext[] = { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0 };
static const uint16_t tinf_dist_base[] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577 };
static const uint8_t tinf_dist_ext[] = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13 };

int tinf_uncompress(uint8_t *dest, const uint8_t *src, int src_len) {
    tinf_data_t d;
    d.source = src; d.tag = 0; d.bitcount = 0;
    uint8_t *out = dest;
    int bfinal = 0;
    while (!bfinal) {
        bfinal = tinf_getbits(&d, 1);
        int btype = tinf_getbits(&d, 2);
        if (btype == 1 || btype == 2) {
            tinf_tree_t lt, dt;
            if (btype == 1) {
                uint8_t lengths[288];
                for (int i = 0; i <= 143; i++) lengths[i] = 8;
                for (int i = 144; i <= 255; i++) lengths[i] = 9;
                for (int i = 256; i <= 279; i++) lengths[i] = 7;
                for (int i = 280; i <= 287; i++) lengths[i] = 8;
                tinf_build_tree(&lt, lengths, 288);
                for (int i = 0; i < 32; i++) lengths[i] = 5;
                tinf_build_tree(&dt, lengths, 32);
            } else {
                int hlit = tinf_getbits(&d, 5) + 257;
                int hdist = tinf_getbits(&d, 5) + 1;
                int hclen = tinf_getbits(&d, 4) + 4;
                uint8_t lengths[320], clent[19] = {0};
                for (int i = 0; i < hclen; i++) clent[tinf_order[i]] = tinf_getbits(&d, 3);
                tinf_tree_t ct; tinf_build_tree(&ct, clent, 19);
                for (int n = 0; n < hlit + hdist; ) {
                    int s = tinf_decode_symbol(&d, &ct);
                    if (s < 16) lengths[n++] = s;
                    else {
                        int r = 0, l = 0;
                        if (s == 16) { r = tinf_getbits(&d, 2) + 3; l = lengths[n-1]; }
                        else if (s == 17) r = tinf_getbits(&d, 3) + 3;
                        else if (s == 18) r = tinf_getbits(&d, 7) + 11;
                        while(r--) lengths[n++] = l;
                    }
                }
                tinf_build_tree(&lt, lengths, hlit);
                tinf_build_tree(&dt, lengths + hlit, hdist);
            }
            while (1) {
                int s = tinf_decode_symbol(&d, &lt);
                if (s < 256) *out++ = s;
                else if (s == 256) break;
                else {
                    s -= 257;
                    int length = tinf_length_base[s] + tinf_getbits(&d, tinf_length_ext[s]);
                    int dist_sym = tinf_decode_symbol(&d, &dt);
                    int dist = tinf_dist_base[dist_sym] + tinf_getbits(&d, tinf_dist_ext[dist_sym]);
                    for (int i = 0; i < length; i++) { out[0] = out[-dist]; out++; }
                }
            }
        }
    }
    return out - dest;
}

/* --- PNG Implementation --- */

static uint32_t png_swap32(uint32_t val) {
    return ((val >> 24) & 0xff) | ((val >> 8) & 0xff00) | ((val << 8) & 0xff0000) | ((val << 24) & 0xff000000);
}

static void png_unfilter(uint8_t *line, uint8_t *prev, int w, int bpp) {
    int f = line[0]; line++;
    for (int i = 0; i < w * bpp; i++) {
        uint8_t a = (i >= bpp) ? line[i-bpp] : 0;
        uint8_t b = prev ? prev[i+1] : 0;
        uint8_t c = (prev && i >= bpp) ? prev[i-bpp+1] : 0;
        switch(f) {
            case 1: line[i] += a; break;
            case 2: line[i] += b; break;
            case 3: line[i] += (a + b) >> 1; break;
            case 4: {
                int p = a + b - c, pa = p-a, pb = p-b, pc = p-c;
                if (pa < 0) pa = -pa; if (pb < 0) pb = -pb; if (pc < 0) pc = -pc;
                line[i] += (pa <= pb && pa <= pc) ? a : (pb <= pc ? b : c);
            } break;
        }
    }
}

// Global buffers for decompression. 
// In a real OS, these should be allocated via a proper mm.
// For now, we use a safe high memory region for BIOS and will be fixed up for UEFI.
// Use safer memory regions for BIOS/UEFI buffers
static uint8_t *decompression_buffer = (uint8_t *)0x1000000; // 16MB
static uint8_t *idat_buffer = (uint8_t *)0x1F00000;         // ~31MB

void krexo_fb_draw_png_rect(krexo_fb_t *fb, uint8_t *data, uint32_t x_start, uint32_t y_start, uint32_t w, uint32_t h) {
    if (data[0] != 0x89 || data[1] != 'P' || data[2] != 'N' || data[3] != 'G') return;
    uint8_t *p = data + 8;
    uint32_t width = 0, height = 0;
    uint8_t bpp = 0;
    int idat_len = 0;
    while (1) {
        uint32_t len = png_swap32(*(uint32_t*)p);
        uint32_t type = *(uint32_t*)(p + 4);
        if (type == 0x52444849) { // IHDR
            width = png_swap32(*(uint32_t*)(p+8));
            height = png_swap32(*(uint32_t*)(p+12));
            int color = p[17];
            if (color == 2) bpp = 3; 
            else if (color == 6) bpp = 4;
            else if (color == 0) bpp = 1; // Grayscale
            else if (color == 3) bpp = 1; // Indexed
            // kprintf("PNG: %dx%d, color=%d, bpp=%d\n", width, height, color, bpp);
        } else if (type == 0x54414449) { // IDAT
            for (uint32_t i = 0; i < len; i++) idat_buffer[idat_len++] = p[8+i];
        } else if (type == 0x444E4549) break; // IEND
        p += len + 12;
    }
    if (bpp == 0) {
        // kprintf("PNG: Unsupported color type\n");
        return;
    }
    int inflated_len = tinf_uncompress(decompression_buffer, idat_buffer + 2, idat_len - 6);
    (void)inflated_len; // May be used for debug

    int stride = width * bpp + 1;

    // Correct approach: Unfilter the entire image first.
    uint8_t *u_line = decompression_buffer;
    uint8_t *u_prev = NULL;
    for (uint32_t i = 0; i < height; i++) {
        png_unfilter(u_line, u_prev, width, bpp);
        u_prev = u_line;
        u_line += stride;
    }

    // Now draw with scaling to fill the screen (respecting the target rectangle)
    for (uint32_t y = y_start; y < y_start + h && y < fb->height; y++) {
        uint32_t src_y = (y * height) / fb->height;
        uint8_t *src_pixels = decompression_buffer + (src_y * stride) + 1;
        
        for (uint32_t x = x_start; x < x_start + w && x < fb->width; x++) {
            uint32_t src_x = (x * width) / fb->width;
            uint8_t *p = src_pixels + (src_x * bpp);
            uint32_t color;
            if (bpp >= 3) {
                color = (p[0] << 16) | (p[1] << 8) | p[2];
            } else {
                // Grayscale or Indexed (as crude grayscale)
                color = (p[0] << 16) | (p[0] << 8) | p[0];
            }
            krexo_fb_put_pixel(fb, x, y, color);
        }
    }
}

void krexo_fb_draw_png(krexo_fb_t *fb, uint8_t *data) {
    krexo_fb_draw_png_rect(fb, data, 0, 0, fb->width, fb->height);
}
