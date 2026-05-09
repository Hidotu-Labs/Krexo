#include <common/config.h>
#include <common/fb.h>
#include <common/kprint.h>

static void menu_draw_centered(krexo_fb_t* fb, int y, const char* str, uint32_t color) {
    int len = 0;
    while (str[len]) len++;
    int x = (fb->width - (len * 8)) / 2;
    krexo_fb_draw_string(fb, x, y, str, color);
}

void menu_draw(krexo_fb_t* fb, krexo_config_t* config, int selected, int timeout) {
    static uint32_t last_bg = 0xFFFFFFFF;
    
    // FULL DRAW ONLY IF BG CHANGED (or first time)
    if (config->background_color != last_bg) {
        krexo_fb_fill_rect(fb, 0, 0, fb->width, fb->height, config->background_color);
        // Title at the top (Static part)
        const char* title = "=== KREXO BOOT MANAGER ===";
        int tw = 26 * 8;
        krexo_fb_draw_string(fb, (fb->width - tw) / 2, 80, title, 0x00FFFF);
        
        // Static subtitle
        menu_draw_centered(fb, 120, "Use Up/Down to select, Enter to boot", 0xAAAAAA);
        last_bg = config->background_color;
    }

    // PARTIAL DRAW (Dirty Rectangles / Stripes)
    // 1. Clear and Redraw Dynamic Entries Area
    int entry_h = 45;
    int total_h = config->count * entry_h;
    int start_y = (fb->height - total_h) / 2 + 30;
    
    // Wipe only the menu area stripe
    krexo_fb_fill_rect(fb, 0, start_y - 20, fb->width, total_h + 40, config->background_color);

    for (int i = 0; i < config->count; i++) {
        int y = start_y + (i * entry_h);
        int len = 0; while (config->entries[i].name[len]) len++;
        int w = len * 8;
        int x = (fb->width - w) / 2;

        if (i == selected) {
            krexo_fb_fill_rect(fb, x - 20, y - 10, w + 40, 32, 0x00AAAA);
            krexo_fb_draw_string(fb, x, y, config->entries[i].name, 0xFFFFFF);
        } else {
            krexo_fb_draw_string(fb, x, y, config->entries[i].name, 0xCCCCCC);
        }
    }

    // 2. Clear and Redraw Timer Area
    krexo_fb_fill_rect(fb, 0, fb->height - 100, fb->width, 40, config->background_color);
    if (timeout > 0) {
        char tbuf[64];
        ksprintf(tbuf, "Automatic boot in %d seconds...", timeout);
        menu_draw_centered(fb, fb->height - 80, tbuf, 0xAAAAAA);
    }
}

int menu_loop(krexo_fb_t* fb, krexo_config_t* config, int (*get_key)(void), void (*delay_ms)(int)) {
    int selected = 0;
    int timeout_ms = config->timeout * 1000;
    int cancel_timeout = (config->timeout == 0);
    
    int last_selected = -1;
    int last_display_sec = -1;

    while (1) {
        int display_sec = cancel_timeout ? 0 : (timeout_ms + 999) / 1000;
        
        // ONLY draw if state changed
        if (selected != last_selected || display_sec != last_display_sec) {
            menu_draw(fb, config, selected, display_sec);
            last_selected = selected;
            last_display_sec = display_sec;
        }
        
        // Poll for key for up to 100ms
        for (int p = 0; p < 10; p++) {
            int key = get_key();
            if (key != 0) {
                cancel_timeout = 1;
                if (key == 1) { // Up
                    selected = (selected > 0) ? selected - 1 : config->count - 1;
                } else if (key == 2) { // Down
                    selected = (selected < config->count - 1) ? selected + 1 : 0;
                } else if (key == 3) { // Enter
                    return selected;
                }
                break; // State changed, exit polling inner loop to redraw
            }
            delay_ms(10);
            if (!cancel_timeout) {
                timeout_ms -= 10;
                if (timeout_ms <= 0) return 0; // Default boot
            }
        }
    }
}
