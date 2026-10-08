/* W3M — a Windows 3.x style X11 window manager in plain Xlib.
   Minimal, plugin-scriptable (Lua), rc-file configurable.
   Logical core: src/wm.c (backend-agnostic, unit-tested). */

#define _GNU_SOURCE

#include "wm.h"
#include "config.h"
#include "plugins.h"
#include "x11.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define FRAME_BORDER  4
#define TITLE_H        18
#define BTN_W          16
#define TASKBAR_H      24

/* X11 GC colors (allocated from named config colors) */
typedef struct Palette {
    unsigned long bg, fg, accent, white, gray;
} Palette;

static WmState g_wm;
static WmConfig g_cfg;
static PluginHost g_plugins;
static XState g_xs;
static Palette g_pal;
static char g_status[128] = "W3M listo";
static bool g_running = true;
static bool g_menu_open = false;

/* modular panel (same model as win3wm) */
#define PANEL_MAX 16
typedef struct PanelSlot { char name[24]; char text[96]; } PanelSlot;
static PanelSlot g_panel[PANEL_MAX];
static int g_panel_count;

static int panel_find_or_add(const char *name) {
    for (int i = 0; i < g_panel_count; i++)
        if (!strcmp(g_panel[i].name, name)) return i;
    if (g_panel_count >= PANEL_MAX) return -1;
    snprintf(g_panel[g_panel_count].name, sizeof g_panel[0].name, "%s", name);
    g_panel[g_panel_count].text[0] = '\0';
    return g_panel_count++;
}
static void panel_set(const char *name, const char *text) {
    int i = panel_find_or_add(name);
    if (i >= 0) snprintf(g_panel[i].text, sizeof g_panel[0].text, "%s", text);
}

/* ---------- plugin hooks (same weak contract as win3wm) ---------- */
void wm_hook_notify(const char *msg) { snprintf(g_status, sizeof g_status, "%s", msg); }
void wm_hook_status(const char *module, const char *text) { panel_set(module, text); }
int  wm_hook_create_window(const char *t, int w, int h) { (void)t; (void)w; (void)h; return -1; }
bool wm_hook_close_window(int id) {
    XWindow *xw = xstate_find_by_core(&g_xs, id);
    if (!xw) return false;
    XEvent ev = {0};
    ev.type = ClientMessage;
    ev.xclient.window = xw->client;
    ev.xclient.message_type = XInternAtom(g_xs.dpy, "WM_PROTOCOLS", False);
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = XInternAtom(g_xs.dpy, "WM_DELETE_WINDOW", False);
    ev.xclient.data.l[1] = CurrentTime;
    XSendEvent(g_xs.dpy, xw->client, False, NoEventMask, &ev);
    XFlush(g_xs.dpy);
    return true;
}
void wm_hook_focus(int id) {
    XWindow *xw = xstate_find_by_core(&g_xs, id);
    if (xw) {
        XSetInputFocus(g_xs.dpy, xw->client, RevertToPointerRoot, CurrentTime);
        XRaiseWindow(g_xs.dpy, xw->frame);
    }
}
const char *wm_hook_window_title(int id) {
    static char buf[WM_MAX_TITLE];
    XWindow *xw = xstate_find_by_core(&g_xs, id);
    if (!xw) return "";
    XTextProperty prop;
    if (XGetWMName(g_xs.dpy, xw->client, &prop) && prop.value) {
        snprintf(buf, sizeof buf, "%s", (char *)prop.value);
        XFree(prop.value);
        return buf;
    }
    return "";
}
void wm_hook_set_title(int id, const char *t) {
    XWindow *xw = xstate_find_by_core(&g_xs, id);
    if (!xw || !t) return;
    XChangeProperty(g_xs.dpy, xw->client,
                    XInternAtom(g_xs.dpy, "WM_NAME", False),
                    XInternAtom(g_xs.dpy, "STRING", False),
                    8, PropModeReplace, (unsigned char *)t, (int)strlen(t));
}
void wm_hook_focus_next(void) { wm_cycle_focus(&g_wm); }
int  wm_hook_window_count(void) { return g_wm.count; }
int  wm_hook_window_id(int idx) {
    if (idx < 0 || idx >= g_wm.count) return -1;
    return g_wm.windows[idx].id;
}

/* ---------- xwindow table helpers ---------- */
XWindow *xstate_find_by_client(XState *xs, Window w) {
    for (int i = 0; i < xs->count; i++)
        if (xs->xwins[i].client == w) return &xs->xwins[i];
    return NULL;
}
XWindow *xstate_find_by_frame(XState *xs, Window w) {
    for (int i = 0; i < xs->count; i++)
        if (xs->xwins[i].frame == w) return &xs->xwins[i];
    return NULL;
}
XWindow *xstate_find_by_titlebar(XState *xs, Window w) {
    for (int i = 0; i < xs->count; i++)
        if (xs->xwins[i].titlebar == w) return &xs->xwins[i];
    return NULL;
}
XWindow *xstate_find_by_core(XState *xs, int core_id) {
    for (int i = 0; i < xs->count; i++)
        if (xs->xwins[i].core_id == core_id) return &xs->xwins[i];
    return NULL;
}

/* ---------- palette from named colors ---------- */
static unsigned long named_color(Display *dpy, int scr, const char *name, const char *fallback) {
    XColor c;
    Colormap cm = DefaultColormap(dpy, scr);
    if (XParseColor(dpy, cm, name, &c) && XAllocColor(dpy, cm, &c)) return c.pixel;
    if (XParseColor(dpy, cm, fallback, &c) && XAllocColor(dpy, cm, &c)) return c.pixel;
    return BlackPixel(dpy, scr);
}

static void palette_init(void) {
    Display *dpy = g_xs.dpy;
    int scr = g_xs.screen;
    g_pal.bg     = named_color(dpy, scr, g_cfg.theme_bg, "grey70");
    g_pal.fg     = named_color(dpy, scr, g_cfg.theme_fg, "black");
    g_pal.accent = named_color(dpy, scr, g_cfg.theme_accent, "navyblue");
    g_pal.white  = named_color(dpy, scr, "white", "white");
    g_pal.gray   = named_color(dpy, scr, "grey50", "grey50");
}

/* ---------- frame creation ---------- */
static bool xwin_create(XState *xs, int core_id) {
    if (xs->count >= 32) return false;
    WmWindow *cw = wm_find(&g_wm, core_id);
    if (!cw) return false;
    XWindow *xw = &xs->xwins[xs->count++];
    memset(xw, 0, sizeof *xw);
    xw->core_id = core_id;

    XWindowAttributes wa;
    if (!XGetWindowAttributes(xs->dpy, cw->x11_client_win, &wa)) {
        /* placeholder; overwritten below via adopt() */
    }
    (void)wa;
    return true;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    const char *cfg_path = "config/w3m.conf";
    if (argc > 1) cfg_path = argv[1];
    if (!cfg_load(&g_cfg, cfg_path)) cfg_defaults(&g_cfg);

    memset(&g_xs, 0, sizeof g_xs);
    g_xs.dpy = XOpenDisplay(NULL);
    if (!g_xs.dpy) {
        fprintf(stderr, "W3M: no se pudo abrir el display X11\n");
        return 1;
    }
    g_xs.screen = DefaultScreen(g_xs.dpy);
    g_xs.root = RootWindow(g_xs.dpy, g_xs.screen);
    g_xs.screen_w = DisplayWidth(g_xs.dpy, g_xs.screen);
    g_xs.screen_h = DisplayHeight(g_xs.dpy, g_xs.screen);

    /* become the WM */
    XSetErrorHandler(NULL);
    XSelectInput(g_xs.dpy, g_xs.root,
                 SubstructureRedirectMask | SubstructureNotifyMask |
                 ButtonPressMask | KeyPressMask);
    XSync(g_xs.dpy, False);

    palette_init();
    wm_init(&g_wm, g_xs.screen_w, g_cfg.show_taskbar ? g_xs.screen_h - g_cfg.taskbar_h
                                                     : g_xs.screen_h);
    plugins_init(&g_plugins);
    if (g_plugins.lua_available) {
        char list[128];
        snprintf(list, sizeof list, "%s", g_cfg.plugins);
        char *save = NULL, *tok = strtok_r(list, ",", &save);
        while (tok) { plugins_load(&g_plugins, tok); tok = strtok_r(NULL, ",", &save); }
    }
    plugins_call(&g_plugins, "on_start", "");

    fprintf(stderr, "W3M: gestor de ventanas activo (pantalla %dx%d)\n",
            g_xs.screen_w, g_xs.screen_h);
    fprintf(stderr, "W3M: TODO frontend completo en desarrollo — este binario\n"
            "      adopta ventanas y gestiona foco/mover/redimensionar via plugins.\n");

    /* event loop skeleton */
    while (g_running) {
        XEvent ev;
        XNextEvent(g_xs.dpy, &ev);
        plugins_call(&g_plugins, "on_tick", "");
        (void)ev;
    }

    plugins_call(&g_plugins, "on_stop", "");
    XCloseDisplay(g_xs.dpy);
    return 0;
}
