#ifndef WM_CONFIG_H
#define WM_CONFIG_H

#include <stdbool.h>

#define CFG_MAX_LINE 256

typedef struct WmConfig {
    int  screen_w;
    int  screen_h;
    int  titlebar_h;
    int  taskbar_h;
    bool show_taskbar;
    char wallpaper[64];
    char theme_fg[32];
    char theme_bg[32];
    char theme_accent[32];
    char plugins[128];        /* comma-separated plugin list */
    char panel_modules[128];   /* panel sections, in order: tasks,clock,date,mem,... */
    int  window_default_w;
    int  window_default_h;
} WmConfig;

void cfg_defaults(WmConfig *cfg);
bool cfg_load(WmConfig *cfg, const char *path);   /* returns false on parse error */
const char *cfg_get(const WmConfig *cfg, const char *key); /* NULL if missing */

#endif
