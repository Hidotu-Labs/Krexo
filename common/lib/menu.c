#include <common/config.h>
#include <common/fb.h>
#include <common/keys.h>
#include <common/kprint.h>
#include <stddef.h>
#include <stdint.h>

static void menu_draw_string_centered(krexo_fb_t *fb, int x_start, int width, int y,
                                      const char *str, uint32_t color) {
  int len = 0;
  while (str[len])
    len++;
  int x = x_start + (width - (len * 8)) / 2;
  krexo_fb_draw_string(fb, x, y, str, color);
}

static void menu_draw_centered(krexo_fb_t *fb, int y, const char *str,
                               uint32_t color) {
  menu_draw_string_centered(fb, 0, fb->width, y, str, color);
}

void menu_draw(krexo_fb_t *fb, krexo_config_t *config, int selected,
               int timeout, uint8_t *bg_data, int force_redraw) {
  static uint32_t last_bg = 0xFFFFFFFF;
  static uint8_t *last_bg_data = NULL;

  // 1. FULL DRAW (Background)
  if (config->background_color != last_bg || bg_data != last_bg_data ||
      force_redraw) {
    if (bg_data) {
      krexo_fb_draw_bmp(fb, bg_data);
    } else {
      krexo_fb_draw_noise_bg(fb, config->background_color);
    }
    last_bg = config->background_color;
    last_bg_data = bg_data;
  }

  // 2. Stylish "Glass" Container
  int box_w = (fb->width > 600) ? 500 : (fb->width * 4 / 5);
  int box_h = (config->count * 45) + 100;
  int box_x = (fb->width - box_w) / 2;
  int box_y = (fb->height - box_h) / 2 - 40;

  // Draw semi-transparent background for the menu
  krexo_fb_draw_rect_transparent(fb, box_x, box_y, box_w, box_h, config->ui_menu_bg, 190);
  krexo_fb_draw_rect_outline(fb, box_x, box_y, box_w, box_h,
                             config->ui_menu_border); // Tokyo blue border

  // Title Bar Effect
  krexo_fb_fill_rect(fb, box_x, box_y, box_w, 35, config->ui_menu_bg);
  krexo_fb_draw_rect_outline(fb, box_x, box_y, box_w, 35, config->ui_menu_border);
  krexo_fb_draw_string(fb, box_x + 20, box_y + 10, "KREXO BOOT MANAGER",
                       config->ui_menu_title);

  // 3. Entries
  int entry_y_start = box_y + 60;
  for (int i = 0; i < config->count; i++) {
    int y_box = entry_y_start + (i * 45) - 8; // Center the 32px box around the entry line
    int y_text = y_box + (32 - 16) / 2;     // Center the 16px font in the 32px box

    if (i == selected) {
      // Selection Glow
      krexo_fb_draw_rect_transparent(fb, box_x + 10, y_box, box_w - 20, 32,
                                     config->ui_menu_sel_bg, 150);
      krexo_fb_draw_rect_outline(fb, box_x + 10, y_box, box_w - 20, 32,
                                 config->ui_menu_title);
      krexo_fb_draw_string(fb, box_x + 30, y_text, config->entries[i].name,
                           config->ui_menu_text_sel);
    } else {
      krexo_fb_draw_string(fb, box_x + 30, y_text, config->entries[i].name,
                           config->ui_menu_text);
    }
  }

  // 4. Footer info
  if (timeout > 0) {
    char tbuf[64];
    ksprintf(tbuf, "Automatic boot in %ds", timeout);
    krexo_fb_draw_string(fb, box_x + 20, box_y + box_h - 30, tbuf, config->ui_menu_accent);
  }

  // External Nav Hint
  menu_draw_centered(fb, fb->height - 60,
                     "Navigation: UP/DOWN / ENTER / E (Edit)", config->ui_menu_help);
}

static void menu_edit_entry(krexo_fb_t *fb, krexo_config_t *config,
                            int entry_idx,
                            int (*get_key)(void), void (*delay_ms)(int)) {
  krexo_boot_entry_t *entry = &config->entries[entry_idx];
  char buffer[256];
  int pos = 0;
  while (entry->cmdline[pos] && pos < 255) {
    buffer[pos] = entry->cmdline[pos];
    pos++;
  }
  buffer[pos] = 0;

  int last_pos = -1;

  while (1) {
    if (pos != last_pos) {
      int y = fb->height - 180;
      // Editor Glass Box
      krexo_fb_draw_rect_transparent(fb, (fb->width / 8), y - 10,
                                     (fb->width * 6 / 8), 100, config->ui_menu_bg, 220);
      krexo_fb_draw_rect_outline(fb, (fb->width / 8), y - 10,
                                 (fb->width * 6 / 8), 100, config->ui_menu_help);

      krexo_fb_draw_string(fb, (fb->width / 8) + 20, y + 3,
                           "EDIT COMMAND LINE:", config->ui_menu_text_sel);

      char display[300];
      ksprintf(display, "> %s ", buffer);
      // Simple underscore cursor
      int dlen = 0;
      while (display[dlen])
        dlen++;
      display[dlen] = '_';
      display[dlen + 1] = 0;

      krexo_fb_draw_string(fb, (fb->width / 8) + 20, y + 32, display,
                           config->ui_menu_text);
      krexo_fb_draw_string(fb, (fb->width / 8) + 20, y + 61,
                           "[ENTER] Save Changes    [ESC] Discard",
                           config->ui_menu_help);
      last_pos = pos;
    }

    int key = get_key();
    if (key != KREXO_KEY_NONE) {
      if (key == KREXO_KEY_ENTER) {
        int i = 0;
        for (; buffer[i] && i < 255; i++)
          entry->cmdline[i] = buffer[i];
        entry->cmdline[i] = 0;
        break;
      } else if (key == KREXO_KEY_ESC) {
        break;
      } else if (key == KREXO_KEY_BACKSPACE || key == '\b') {
        if (pos > 0) {
          buffer[--pos] = 0;
        }
      } else if (key >= 32 && key <= 126 && pos < 255) {
        buffer[pos++] = (char)key;
        buffer[pos] = 0;
      }
    }
    delay_ms(10);
  }
}

int menu_loop(krexo_fb_t *fb, krexo_config_t *config, int (*get_key)(void),
              void (*delay_ms)(int), uint8_t *bg_data) {
  int selected = 0;
  int timeout_ms = config->timeout * 1000;
  int cancel_timeout = (config->timeout == 0);

  int last_selected = -1;
  int last_display_sec = -1;

  while (1) {
    int display_sec = cancel_timeout ? 0 : (timeout_ms + 999) / 1000;

    // ONLY draw if state changed
    if (selected != last_selected || display_sec != last_display_sec ||
        last_selected == -2) {
      menu_draw(fb, config, selected, display_sec, bg_data,
                (last_selected == -2));
      last_selected = selected;
      last_display_sec = display_sec;
    }

    // Poll for key for up to 100ms
    for (int p = 0; p < 10; p++) {
      int key = get_key();
      if (key != 0) {
        cancel_timeout = 1;
        last_selected = -2; // Force redraw to clear timer immediately
        if (key == KREXO_KEY_UP) {
          selected = (selected > 0) ? selected - 1 : config->count - 1;
        } else if (key == KREXO_KEY_DOWN) {
          selected = (selected < config->count - 1) ? selected + 1 : 0;
        } else if (key == KREXO_KEY_ENTER) {
          return selected;
        } else if (key == KREXO_KEY_EDIT) {
          menu_edit_entry(fb, config, selected, get_key, delay_ms);
          last_selected = -2; // Force redraw and flag as forced
        }
        break; // State changed, exit polling inner loop to redraw
      }
      delay_ms(10);
      if (!cancel_timeout) {
        timeout_ms -= 10;
        if (timeout_ms <= 0)
          return 0; // Default boot
      }
    }
  }
}
