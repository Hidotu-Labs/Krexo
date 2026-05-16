#ifndef KREXO_CONFIG_H
#define KREXO_CONFIG_H

#include <stdint.h>

#define MAX_CONFIG_ENTRIES 8

typedef struct {
    char name[64];
    char kernel_path[128];
    char cmdline[256];
} krexo_boot_entry_t;

typedef struct {
    krexo_boot_entry_t entries[MAX_CONFIG_ENTRIES];
    int count;
    int timeout;
    uint32_t background_color;
    char background_image_path[128];

    // UI Colors
    uint32_t ui_menu_bg;
    uint32_t ui_menu_border;
    uint32_t ui_menu_title;
    uint32_t ui_menu_text;
    uint32_t ui_menu_text_sel;
    uint32_t ui_menu_sel_bg;
    uint32_t ui_menu_accent;
    uint32_t ui_menu_help;
} krexo_config_t;

// Parses the raw config text into a structured list of entries
int config_parse(const char* data, krexo_config_t* config);

#endif // KREXO_CONFIG_H
