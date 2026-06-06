#include <common/config.h>
#include <stddef.h>
#include <stdint.h>

static void k_strncpy(char *dest, const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n && src[i] != '\0' && src[i] != '\n' && src[i] != '\r';
       i++) {
    dest[i] = src[i];
  }
  for (; i < n; i++) {
    dest[i] = '\0';
  }
}

static int k_strncmp(const char *s1, const char *s2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (s1[i] != s2[i])
      return 1;
    if (s1[i] == 0)
      return 0;
  }
  return 0;
}

static uint32_t parse_hex(const char *s) {
  if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    s += 2;
  uint32_t val = 0;
  while (*s) {
    uint8_t byte = *s;
    if (byte >= '0' && byte <= '9')
      byte = byte - '0';
    else if (byte >= 'a' && byte <= 'f')
      byte = byte - 'a' + 10;
    else if (byte >= 'A' && byte <= 'F')
      byte = byte - 'A' + 10;
    else
      break;
    val = (val << 4) | (byte & 0xF);
    s++;
  }
  return val;
}

static int parse_int(const char *s) {
  int val = 0;
  while (*s >= '0' && *s <= '9') {
    val = val * 10 + (*s - '0');
    s++;
  }
  return val;
}

int config_parse(const char *data, krexo_config_t *config) {
  config->count = 0;
  config->timeout = 5;
  config->background_color = 0x1a1a1a;

  // Default Retro Coffee Theme
  config->ui_menu_bg = 0x151210;
  config->ui_menu_border = 0x5c4033;
  config->ui_menu_title = 0xd69e2e;
  config->ui_menu_text = 0xedc9af;
  config->ui_menu_text_sel = 0xffb13b;
  config->ui_menu_sel_bg = 0x2b1e15;
  config->ui_menu_accent = 0x8b4513;
  config->ui_menu_help = 0x5c4033;

  const char *ptr = data;
  krexo_boot_entry_t *current = NULL;

  while (*ptr) {
    // Skip leading whitespace
    while (*ptr == ' ' || *ptr == '\r' || *ptr == '\n')
      ptr++;
    if (!*ptr)
      break;

    if (*ptr == ':') {
      if (config->count < MAX_CONFIG_ENTRIES) {
        current = &config->entries[config->count++];
        ptr++; // Skip ':'
        int i = 0;
        while (*ptr && *ptr != '\n' && *ptr != '\r' && i < 63)
          current->name[i++] = *ptr++;
        current->name[i] = 0;
      }
    } else {
      if (k_strncmp(ptr, "BACKGROUND_COLOR=", 17) == 0) {
        ptr += 17;
        config->background_color = parse_hex(ptr);
      } else if (k_strncmp(ptr, "MENU_BG=", 8) == 0) {
        ptr += 8;
        config->ui_menu_bg = parse_hex(ptr);
      } else if (k_strncmp(ptr, "MENU_BORDER=", 12) == 0) {
        ptr += 12;
        config->ui_menu_border = parse_hex(ptr);
      } else if (k_strncmp(ptr, "MENU_TITLE=", 11) == 0) {
        ptr += 11;
        config->ui_menu_title = parse_hex(ptr);
      } else if (k_strncmp(ptr, "MENU_TEXT=", 10) == 0) {
        ptr += 10;
        config->ui_menu_text = parse_hex(ptr);
      } else if (k_strncmp(ptr, "MENU_TEXT_SEL=", 14) == 0) {
        ptr += 14;
        config->ui_menu_text_sel = parse_hex(ptr);
      } else if (k_strncmp(ptr, "MENU_SEL_BG=", 12) == 0) {
        ptr += 12;
        config->ui_menu_sel_bg = parse_hex(ptr);
      } else if (k_strncmp(ptr, "MENU_ACCENT=", 12) == 0) {
        ptr += 12;
        config->ui_menu_accent = parse_hex(ptr);
      } else if (k_strncmp(ptr, "MENU_HELP=", 10) == 0) {
        ptr += 10;
        config->ui_menu_help = parse_hex(ptr);
      } else if (k_strncmp(ptr, "TIMEOUT=", 8) == 0) {
        ptr += 8;
        config->timeout = parse_int(ptr);
      } else if (k_strncmp(ptr, "BACKGROUND_IMAGE=", 17) == 0) {
        ptr += 17;
        k_strncpy(config->background_image_path, ptr, 127);
      } else if (current) {
        if (k_strncmp(ptr, "KERNEL_PATH=", 12) == 0) {
          ptr += 12;
          k_strncpy(current->kernel_path, ptr, 127);
        } else if (k_strncmp(ptr, "CMDLINE=", 8) == 0) {
          ptr += 8;
          k_strncpy(current->cmdline, ptr, 255);
        }
      }
    }

    while (*ptr && *ptr != '\n' && *ptr != '\r')
      ptr++;
  }
  return 0;
}
