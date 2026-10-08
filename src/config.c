#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char g_lines[32][CFG_MAX_LINE];
static int  g_line_count;

void cfg_defaults(WmConfig *cfg) {
    memset(cfg, 0, sizeof *cfg);
    cfg->screen_w = 1024;
    cfg->screen_h = 768;
    cfg->titlebar_h = 18;
    cfg->taskbar_h = 24;
    cfg->show_taskbar = true;
    snprintf(cfg->wallpaper, sizeof cfg->wallpaper, "teal");
    snprintf(cfg->theme_fg, sizeof cfg->theme_fg, "black");
    snprintf(cfg->theme_bg, sizeof cfg->theme_bg, "silver");
    snprintf(cfg->theme_accent, sizeof cfg->theme_accent, "navy");
    snprintf(cfg->plugins, sizeof cfg->plugins, "clock");
    snprintf(cfg->panel_modules, sizeof cfg->panel_modules, "tasks,clock,date");
    cfg->window_default_w = 320;
    cfg->window_default_h = 200;
}

/* minimal key=value parser, `#` starts a comment */
bool cfg_load(WmConfig *cfg, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return false;
    cfg_defaults(cfg);
    g_line_count = 0;
    char line[CFG_MAX_LINE];
    while (fgets(line, sizeof line, f)) {
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';
        char *nl = strpbrk(line, "\r\n");
        if (nl) *nl = '\0';
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line, *val = eq + 1;
        while (*key == ' ' || *key == '\t') key++;
        while (*val == ' ' || *val == '\t') val++;
        char *end = key + strlen(key);
        while (end > key && (end[-1] == ' ' || end[-1] == '\t')) *--end = '\0';
        end = val + strlen(val);
        while (end > val && (end[-1] == ' ' || end[-1] == '\t')) *--end = '\0';
        if (g_line_count < 32) {
            snprintf(g_lines[g_line_count], CFG_MAX_LINE, "%s=%s", key, val);
            g_line_count++;
        }
        if (!strcmp(key, "screen_w")) cfg->screen_w = atoi(val);
        else if (!strcmp(key, "screen_h")) cfg->screen_h = atoi(val);
        else if (!strcmp(key, "titlebar_h")) cfg->titlebar_h = atoi(val);
        else if (!strcmp(key, "taskbar_h")) cfg->taskbar_h = atoi(val);
        else if (!strcmp(key, "show_taskbar")) cfg->show_taskbar = !strcmp(val, "true");
        else if (!strcmp(key, "wallpaper")) snprintf(cfg->wallpaper, sizeof cfg->wallpaper, "%s", val);
        else if (!strcmp(key, "theme_fg")) snprintf(cfg->theme_fg, sizeof cfg->theme_fg, "%s", val);
        else if (!strcmp(key, "theme_bg")) snprintf(cfg->theme_bg, sizeof cfg->theme_bg, "%s", val);
        else if (!strcmp(key, "theme_accent")) snprintf(cfg->theme_accent, sizeof cfg->theme_accent, "%s", val);
        else if (!strcmp(key, "plugins")) snprintf(cfg->plugins, sizeof cfg->plugins, "%s", val);
        else if (!strcmp(key, "panel_modules")) snprintf(cfg->panel_modules, sizeof cfg->panel_modules, "%s", val);
        else if (!strcmp(key, "window_default_w")) cfg->window_default_w = atoi(val);
        else if (!strcmp(key, "window_default_h")) cfg->window_default_h = atoi(val);
    }
    fclose(f);
    return true;
}

const char *cfg_get(const WmConfig *cfg, const char *key) {
    (void)cfg;
    size_t klen = strlen(key);
    for (int i = 0; i < g_line_count; i++)
        if (!strncmp(g_lines[i], key, klen) && g_lines[i][klen] == '=')
            return g_lines[i] + klen + 1;
    return NULL;
}
