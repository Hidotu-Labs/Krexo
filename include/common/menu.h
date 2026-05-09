#ifndef KREXO_MENU_H
#define KREXO_MENU_H

#include <common/config.h>
#include <common/fb.h>

// Draws the boot menu
void menu_draw(krexo_fb_t* fb, krexo_config_t* config, int selected);

// Runs the interactive menu loop. Returns the index of the selected entry.
int menu_loop(krexo_fb_t* fb, krexo_config_t* config, int (*get_key)(void), void (*delay_ms)(int));

#endif // KREXO_MENU_H
