/* W3M — a Windows 3.x style X11 window manager in plain Xlib.
   Reparenting WM: each client gets a Win 3.x decorated frame.
   Logical core: src/wm.c (backend-agnostic, unit-tested). */

#define _GNU_SOURCE

#include "wm.h"
#include "config.h"
#include "plugins.h"

#if __has_include(<X11/Xlib.h>)
#include <X11/Xlib.h>
#else
#include "../tests/x11_stub.h"
#endif
#if __has_include(<X11/Xutil.h>)
#include <X11/Xutil.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define FRAME_BD    4          /* decoration border */
#define TITLE_H     18         /* title bar height (matches core hit-testing) */
#define BTN_W       16
#define TASKBAR_BTN_W 110

typedef struct XWin {
    Window client, frame;
    int core_id;
} XWin;

typedef struct XState {
    Display *dpy;
    Window root, taskbar;
    int scr, sw, sh;
    GC gc;
    XFontStruct *font;
    unsigned long c_bg, c_fg, c_accent, c_white, c_gray;
    XWin wins[32];
    int count;
    Window menu_win;
} XState;

static WmState g_wm;
static WmConfig g_cfg;
static PluginHost g_plugins;
static XState X;

/* modular panel: slots filled by plugins via wm.status() */
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

static char g_status[128] = "W3M listo";

/* ---------- plugin hooks ---------- */
void wm_hook_notify(const char *msg) { snprintf(g_status, sizeof g_status, "%s", msg); }
void wm_hook_status(const char *m, const char *t) { panel_set(m, t); }
int  wm_hook_create_window(const char *t, int w, int h) { (void)t; (void)w; (void)h; return -1; }

bool wm_hook_close_window(int id) {
    for (int i = 0; i < X.count; i++) {
        if (X.wins[i].core_id != id) continue;
        Atom wm_protocols = XInternAtom(X.dpy, "WM_PROTOCOLS", False);
        Atom wm_delete = XInternAtom(X.dpy, "WM_DELETE_WINDOW", False);
        int n = 0;
        Atom *protocols = XListProtocols(X.dpy, X.wins[i].client, &n);
        bool supports_delete = false;
        if (protocols) {
            for (int k = 0; k < n; k++)
                if (protocols[k] == wm_delete) supports_delete = true;
            XFree(protocols);
        }
        if (supports_delete) {
            XEvent ev = {0};
            ev.type = ClientMessage;
            ev.xclient.window = X.wins[i].client;
            ev.xclient.message_type = wm_protocols;
            ev.xclient.format = 32;
            ev.xclient.data.l[0] = wm_delete;
            ev.xclient.data.l[1] = CurrentTime;
            XSendEvent(X.dpy, X.wins[i].client, False, NoEventMask, &ev);
        } else {
            XDestroyWindow(X.dpy, X.wins[i].client);
        }
        return true;
    }
    return false;
}

static XWin *xwin_by_core(int id) {
    for (int i = 0; i < X.count; i++)
        if (X.wins[i].core_id == id) return &X.wins[i];
    return NULL;
}
static XWin *xwin_by_client(Window w) {
    for (int i = 0; i < X.count; i++)
        if (X.wins[i].client == w) return &X.wins[i];
    return NULL;
}
static XWin *xwin_by_frame(Window w) {
    for (int i = 0; i < X.count; i++)
        if (X.wins[i].frame == w) return &X.wins[i];
    return NULL;
}

void wm_hook_focus(int id) {
    XWin *xw = xwin_by_core(id);
    if (!xw) return;
    XSetInputFocus(X.dpy, xw->client, RevertToPointerRoot, CurrentTime);
    XRaiseWindow(X.dpy, xw->frame);
    /* redraw frame (focus color) + taskbar */
    XClearArea(X.dpy, xw->frame, 0, 0, 0, 0, True);
    if (X.taskbar) XClearArea(X.dpy, X.taskbar, 0, 0, 0, 0, True);
}
const char *wm_hook_window_title(int id) {
    static char buf[WM_MAX_TITLE];
    XWin *xw = xwin_by_core(id);
    if (!xw) return "";
    XTextProperty prop;
    buf[0] = '\0';
    if (XGetWMName(X.dpy, xw->client, &prop) && prop.value) {
        snprintf(buf, sizeof buf, "%s", (char *)prop.value);
        XFree(prop.value);
    }
    return buf;
}
void wm_hook_set_title(int id, const char *t) {
    XWin *xw = xwin_by_core(id);
    if (!xw || !t) return;
    XStoreName(X.dpy, xw->client, t);
    XClearArea(X.dpy, xw->frame, 0, 0, 0, 0, True);
}
void wm_hook_focus_next(void) { wm_cycle_focus(&g_wm); }
int  wm_hook_window_count(void) { return g_wm.count; }
int  wm_hook_window_id(int idx) {
    if (idx < 0 || idx >= g_wm.count) return -1;
    return g_wm.windows[idx].id;
}

/* ---------- frame geometry mapping ---------- */
/* core box == frame geometry (x,y,w,h). client sits inside at
   (FRAME_BD, TITLE_H + FRAME_BD) with size (w - 2*FRAME_BD, h - TITLE_H - 2*FRAME_BD) */

static void frame_to_client(const WmRect *box, XWindowChanges *xc, unsigned *mask) {
    *mask = CWX | CWY | CWWidth | CWHeight;
    xc->x = FRAME_BD;
    xc->y = TITLE_H + FRAME_BD;
    xc->width  = box->w - 2 * FRAME_BD;
    xc->height = box->h - TITLE_H - 2 * FRAME_BD;
}

static void apply_core_geometry(int core_id) {
    XWin *xw = xwin_by_core(core_id);
    WmWindow *cw = wm_find(&g_wm, core_id);
    if (!xw || !cw) return;
    XMoveResizeWindow(X.dpy, xw->frame, cw->box.x, cw->box.y,
                      (unsigned)cw->box.w, (unsigned)cw->box.h);
    XWindowChanges xc;
    unsigned mask;
    frame_to_client(&cw->box, &xc, &mask);
    XConfigureWindow(X.dpy, xw->client, mask, &xc);
}

/* ---------- drawing: frame ---------- */
static void draw_frame(XWin *xw) {
    WmWindow *cw = wm_find(&g_wm, xw->core_id);
    if (!cw) return;
    int w = cw->box.w, h = cw->box.h;

    /* face + bevel */
    XSetForeground(X.dpy, X.gc, X.c_bg);
    XFillRectangle(X.dpy, xw->frame, X.gc, 0, 0, w, h);
    XSetForeground(X.dpy, X.gc, X.c_white);
    XDrawLine(X.dpy, xw->frame, X.gc, 0, 0, w - 1, 0);
    XDrawLine(X.dpy, xw->frame, X.gc, 0, 0, 0, h - 1);
    XSetForeground(X.dpy, X.gc, X.c_gray);
    XDrawLine(X.dpy, xw->frame, X.gc, w - 1, 0, w - 1, h - 1);
    XDrawLine(X.dpy, xw->frame, X.gc, 0, h - 1, w - 1, h - 1);

    /* title bar: accent if focused, gray if not */
    XSetForeground(X.dpy, X.gc, cw->focused ? X.c_accent : X.c_gray);
    XFillRectangle(X.dpy, xw->frame, X.gc, FRAME_BD, FRAME_BD,
                   w - 2 * FRAME_BD, TITLE_H);

    /* title text */
    const char *title = wm_hook_window_title(xw->core_id);
    if (title && title[0]) {
        XSetForeground(X.dpy, X.gc, X.c_white);
        XDrawString(X.dpy, xw->frame, X.gc, FRAME_BD + 4, FRAME_BD + 13,
                    title, (int)strlen(title));
    }

    /* buttons: max ^, min _, close x (same layout the core hit-tests) */
    int bx = w - FRAME_BD - BTN_W + 1;
    int by = FRAME_BD + 1;
    const char *glyphs[3] = {"^", "_", "x"};
    for (int k = 0; k < 3; k++) {
        XSetForeground(X.dpy, X.gc, X.c_bg);
        XFillRectangle(X.dpy, xw->frame, X.gc, bx, by, BTN_W - 2, TITLE_H - 4);
        XSetForeground(X.dpy, X.gc, X.c_white);
        XDrawLine(X.dpy, xw->frame, X.gc, bx, by, bx + BTN_W - 3, by);
        XDrawLine(X.dpy, xw->frame, X.gc, bx, by, bx, by + TITLE_H - 5);
        XSetForeground(X.dpy, X.gc, X.c_fg);
        XDrawString(X.dpy, xw->frame, X.gc, bx + 5, by + 12,
                    glyphs[k], 1);
        bx -= BTN_W + 2;
    }
}

/* ---------- drawing: taskbar ---------- */
static void draw_taskbar(void) {
    if (!X.taskbar || !g_cfg.show_taskbar) return;
    int th = g_cfg.taskbar_h;
    int tw = X.sw;

    XSetForeground(X.dpy, X.gc, X.c_bg);
    XFillRectangle(X.dpy, X.taskbar, X.gc, 0, 0, tw, th);
    XSetForeground(X.dpy, X.gc, X.c_white);
    XDrawLine(X.dpy, X.taskbar, X.gc, 0, 0, tw - 1, 0);

    XSetForeground(X.dpy, X.gc, X.c_fg);
    XDrawString(X.dpy, X.taskbar, X.gc, 6, 15, "W3M", 3);

    /* task buttons */
    int x = 70;
    for (int i = 0; i < g_wm.count && x + TASKBAR_BTN_W < X.sw - 100; i++) {
        WmWindow *cw = &g_wm.windows[i];
        XSetForeground(X.dpy, X.gc,
                       (!cw->minimized && cw->focused) ? X.c_accent : X.c_gray);
        XFillRectangle(X.dpy, X.taskbar, X.gc, x, 3, TASKBAR_BTN_W, th - 6);
        XSetForeground(X.dpy, X.gc, X.c_white);
        const char *title = wm_hook_window_title(cw->id);
        char label[16];
        snprintf(label, sizeof label, "%s%.8s", cw->minimized ? "[+] " : "",
                 title ? title : "");
        XDrawString(X.dpy, X.taskbar, X.gc, x + 4, 15, label, (int)strlen(label));
        x += TASKBAR_BTN_W + 4;
    }

    /* panel modules: right-aligned, in reverse config order */
    char list[128];
    snprintf(list, sizeof list, "%s", g_cfg.panel_modules);
    char *save = NULL;
    char *names[16];
    int nn = 0;
    for (char *tok = strtok_r(list, ",", &save); tok && nn < 16;
         tok = strtok_r(NULL, ",", &save))
        names[nn++] = tok;

    int right = X.sw - 8;
    for (int i = 0; i < nn; i++) {
        const char *mod = names[nn - 1 - i];   /* last module drawn first */
        char buf[96] = "";
        if (!strcmp(mod, "clock")) {
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            strftime(buf, sizeof buf, "%H:%M", tm);
        } else if (!strcmp(mod, "date")) {
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            strftime(buf, sizeof buf, "%d/%m/%Y", tm);
        } else {
            for (int k = 0; k < g_panel_count; k++)
                if (!strcmp(g_panel[k].name, mod) && g_panel[k].text[0]) {
                    snprintf(buf, sizeof buf, "%s", g_panel[k].text);
                    break;
                }
        }
        if (!buf[0] || !strcmp(mod, "tasks")) continue;
        int len = XTextWidth(X.font, buf, (int)strlen(buf));
        XSetForeground(X.dpy, X.gc, X.c_fg);
        XDrawString(X.dpy, X.taskbar, X.gc, right - len, 15, buf, (int)strlen(buf));
        right -= len + 16;
    }
    int slen = XTextWidth(X.font, g_status, (int)strlen(g_status));
    XSetForeground(X.dpy, X.gc, X.c_fg);
    XDrawString(X.dpy, X.taskbar, X.gc, right - slen, 15,
                g_status, (int)strlen(g_status));
}

/* ---------- adopt (reparent) a new client ---------- */
static void adopt(Window client, XWindowAttributes *wa) {
    if (X.count >= 32) return;

    int cw = wa->width  > 0 ? wa->width  : g_cfg.window_default_w;
    int ch = wa->height > 0 ? wa->height : g_cfg.window_default_h;
    if (cw < 80) cw = 80;
    if (ch < 40) ch = 40;
    if (cw > X.sw - 40) cw = X.sw - 40;
    if (ch > g_wm.screen_h - 40) ch = g_wm.screen_h - 40;

    int core_id = wm_create_window(&g_wm, "Ventana", cw + 2 * FRAME_BD,
                                   ch + TITLE_H + 2 * FRAME_BD);
    if (core_id < 0) return;
    WmWindow *cw2 = wm_find(&g_wm, core_id);

    Window frame = XCreateSimpleWindow(X.dpy, X.root,
                                       cw2->box.x, cw2->box.y,
                                       (unsigned)cw2->box.w, (unsigned)cw2->box.h,
                                       0, X.c_bg, X.c_bg);
    XSelectInput(X.dpy, frame,
                 ExposureMask | ButtonPressMask | ButtonReleaseMask |
                 SubstructureNotifyMask | SubstructureRedirectMask);

    XWin *xw = &X.wins[X.count++];
    xw->client = client;
    xw->frame = frame;
    xw->core_id = core_id;

    /* reparent: client inside frame, below the title bar */
    XWindowChanges xc;
    unsigned mask;
    frame_to_client(&cw2->box, &xc, &mask);
    XReparentWindow(X.dpy, client, frame, xc.x, xc.y);
    XConfigureWindow(X.dpy, client, mask, &xc);

    XAddToSaveSet(X.dpy, client);
    XMapWindow(X.dpy, frame);
    XMapWindow(X.dpy, client);
    wm_focus(&g_wm, core_id);
    plugins_call(&g_plugins, "on_window_focused", "ds", core_id,
                 wm_hook_window_title(core_id));
    if (X.taskbar) XClearArea(X.dpy, X.taskbar, 0, 0, 0, 0, True);
}

static void unmanage(XWin *xw, bool destroyed) {
    WmWindow *cw = wm_find(&g_wm, xw->core_id);
    if (cw) {
        int id = xw->core_id;
        if (!destroyed) {
            XUnmapWindow(X.dpy, xw->client);
            XReparentWindow(X.dpy, xw->client, X.root, cw->box.x, cw->box.y);
            XRemoveFromSaveSet(X.dpy, xw->client);
        }
        wm_close_window(&g_wm, id);
        plugins_call(&g_plugins, "on_window_closed", "d", id);
    }
    XDestroyWindow(X.dpy, xw->frame);
    int idx = (int)(xw - X.wins);
    memmove(&X.wins[idx], &X.wins[idx + 1],
            (size_t)(X.count - idx - 1) * sizeof(XWin));
    X.count--;
    if (X.taskbar) XClearArea(X.dpy, X.taskbar, 0, 0, 0, 0, True);
}

/* ---------- interactive drag (pointer grab loop) ---------- */
static void drag_loop(int core_id, int hit) {
    WmWindow *cw = wm_find(&g_wm, core_id);
    XWin *xw = xwin_by_core(core_id);
    if (!cw || !xw) return;

    Window root_ret, child_ret;
    int rx, ry, wx, wy;
    unsigned mask;
    XQueryPointer(X.dpy, X.root, &root_ret, &child_ret, &rx, &ry, &wx, &wy, &mask);

    int start_px = rx, start_py = ry;
    int orig_x = cw->box.x, orig_y = cw->box.y;
    int orig_w = cw->box.w, orig_h = cw->box.h;

    XGrabPointer(X.dpy, X.root, False,
                ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

    XEvent ev;
    for (;;) {
        XMaskEvent(X.dpy, ButtonReleaseMask | PointerMotionMask, &ev);
        if (ev.type == ButtonRelease) break;
        int dx = ev.xmotion.x_root - start_px;
        int dy = ev.xmotion.y_root - start_py;
        if (hit == WM_HIT_TITLE) {
            cw->box.x = orig_x + dx;
            cw->box.y = orig_y + dy;
            wm_constrain(&g_wm);
            XMoveWindow(X.dpy, xw->frame, cw->box.x, cw->box.y);
        } else {
            /* resize: apply delta per edge, reusing core semantics */
            WmRect b = {orig_x, orig_y, orig_w, orig_h};
            switch (hit) {
            case WM_HIT_EDGE_L:  b.x += dx; b.w -= dx; break;
            case WM_HIT_EDGE_R:  b.w += dx; break;
            case WM_HIT_EDGE_T:  b.y += dy; b.h -= dy; break;
            case WM_HIT_EDGE_B:  b.h += dy; break;
            case WM_HIT_CORNER_TL: b.x += dx; b.w -= dx; b.y += dy; b.h -= dy; break;
            case WM_HIT_CORNER_TR: b.w += dx; b.y += dy; b.h -= dy; break;
            case WM_HIT_CORNER_BL: b.x += dx; b.w -= dx; b.h += dy; break;
            default: b.w += dx; b.h += dy; break;  /* BR + body fallback */
            }
            cw->box = b;
            wm_constrain(&g_wm);
            XMoveResizeWindow(X.dpy, xw->frame, cw->box.x, cw->box.y,
                             (unsigned)cw->box.w, (unsigned)cw->box.h);
            XWindowChanges xc;
            unsigned mask2;
            frame_to_client(&cw->box, &xc, &mask2);
            XConfigureWindow(X.dpy, xw->client, mask2, &xc);
        }
    }
    XUngrabPointer(X.dpy, CurrentTime);
    if (cw->maximized && hit != WM_HIT_TITLE) {
        cw->maximized = false;   /* manual resize un-maximizes */
    }
}

/* ---------- taskbar clicks ---------- */
static void taskbar_click(int px, int py) {
    (void)py;
    if (px < 70) {
        /* start area: no menu yet; let plugins know via status */
        snprintf(g_status, sizeof g_status, "W3M %d ventanas", g_wm.count);
        return;
    }
    int x = 70;
    for (int i = 0; i < g_wm.count; i++) {
        if (px >= x && px <= x + TASKBAR_BTN_W) {
            int id = g_wm.windows[i].id;
            WmWindow *cw = wm_find(&g_wm, id);
            if (cw->minimized || !cw->focused) {
                if (cw->minimized) wm_toggle_minimize(&g_wm, id);
                wm_focus(&g_wm, id);
                XMapWindow(X.dpy, xwin_by_core(id)->frame);
                XMapWindow(X.dpy, xwin_by_core(id)->client);
            } else {
                wm_toggle_minimize(&g_wm, id);
                XUnmapWindow(X.dpy, xwin_by_core(id)->frame);
            }
            XClearArea(X.dpy, X.taskbar, 0, 0, 0, 0, True);
            return;
        }
        x += TASKBAR_BTN_W + 4;
    }
}

/* ---------- main ---------- */
int main(int argc, char **argv) {
    const char *cfg_path = "config/w3m.conf";
    if (argc > 1) cfg_path = argv[1];
    if (!cfg_load(&g_cfg, cfg_path)) cfg_defaults(&g_cfg);

    X.dpy = XOpenDisplay(NULL);
    if (!X.dpy) {
        fprintf(stderr, "W3M: no se pudo abrir el display X11\n");
        return 1;
    }
    X.scr = DefaultScreen(X.dpy);
    X.root = RootWindow(X.dpy, X.scr);
    X.sw = DisplayWidth(X.dpy, X.scr);
    X.sh = DisplayHeight(X.dpy, X.scr);

    /* error handler: ignore redirect failures while we take control */
    XSetIOErrorHandler(NULL);

    Colormap cm = DefaultColormap(X.dpy, X.scr);
    XColor col, exact;
    unsigned long alloc_named(const char *n, const char *fb) {
        if (XAllocNamedColor(X.dpy, cm, n, &col, &exact)) return col.pixel;
        if (XAllocNamedColor(X.dpy, cm, fb, &col, &exact)) return col.pixel;
        return BlackPixel(X.dpy, X.scr);
    }
    X.c_bg     = alloc_named(g_cfg.theme_bg, "grey70");
    X.c_fg     = alloc_named(g_cfg.theme_fg, "black");
    X.c_accent = alloc_named(g_cfg.theme_accent, "navyblue");
    X.c_white  = alloc_named("white", "white");
    X.c_gray   = alloc_named("grey50", "grey50");

    X.gc = XCreateGC(X.dpy, X.root, 0, NULL);
    X.font = XLoadQueryFont(X.dpy, "fixed");
    if (!X.font) {
        fprintf(stderr, "W3M: no se pudo cargar la fuente 'fixed'\n");
        return 1;
    }
    XSetFont(X.dpy, X.gc, X.font->fid);

    /* become the WM on this screen */
    XSelectInput(X.dpy, X.root,
                 SubstructureRedirectMask | SubstructureNotifyMask |
                 KeyPressMask | ButtonPressMask);
    /* Alt+Tab */
    XGrabKey(X.dpy, XKeysymToKeycode(X.dpy, XK_Tab), Mod1Mask, X.root,
             True, GrabModeAsync, GrabModeAsync);
    XSync(X.dpy, False);

    wm_init(&g_wm, X.sw, g_cfg.show_taskbar ? X.sh - g_cfg.taskbar_h : X.sh);

    /* taskbar */
    if (g_cfg.show_taskbar) {
        X.taskbar = XCreateSimpleWindow(X.dpy, X.root,
                                        0, X.sh - g_cfg.taskbar_h,
                                        (unsigned)X.sw, (unsigned)g_cfg.taskbar_h,
                                        0, X.c_bg, X.c_bg);
        XSelectInput(X.dpy, X.taskbar, ExposureMask | ButtonPressMask);
        XMapWindow(X.dpy, X.taskbar);
    }

    plugins_init(&g_plugins);
    if (g_plugins.lua_available) {
        char list[128];
        snprintf(list, sizeof list, "%s", g_cfg.plugins);
        char *save = NULL;
        for (char *tok = strtok_r(list, ",", &save); tok;
             tok = strtok_r(NULL, ",", &save))
            plugins_load(&g_plugins, tok);
    }
    plugins_call(&g_plugins, "on_start", "");
    fprintf(stderr, "W3M: gestor activo en pantalla %dx%d (%d ventanas manejadas max 32)\n",
            X.sw, X.sh, 32);

    time_t last_sec = 0;
    for (;;) {
        while (XPending(X.dpy)) {
            XEvent ev;
            XNextEvent(X.dpy, &ev);
            switch (ev.type) {
            case MapRequest: {
                XWindowAttributes wa;
                if (XGetWindowAttributes(X.dpy, ev.xmaprequest.window, &wa)
                    && !xwin_by_client(ev.xmaprequest.window)
                    && wa.override_redirect == False)
                    adopt(ev.xmaprequest.window, &wa);
                break;
            }
            case ConfigureRequest: {
                XWin *xw = xwin_by_client(ev.xconfigurerequest.window);
                if (xw) {
                    /* honor client size wishes by growing the frame */
                    WmWindow *cw = wm_find(&g_wm, xw->core_id);
                    if (cw && (ev.xconfigurerequest.value_mask & CWWidth
                            || ev.xconfigurerequest.value_mask & CWHeight)) {
                        int nw = cw->box.w, nh = cw->box.h;
                        if (ev.xconfigurerequest.value_mask & CWWidth)
                            nw = ev.xconfigurerequest.width + 2 * FRAME_BD;
                        if (ev.xconfigurerequest.value_mask & CWHeight)
                            nh = ev.xconfigurerequest.height + TITLE_H + 2 * FRAME_BD;
                        cw->box.w = nw; cw->box.h = nh;
                        wm_constrain(&g_wm);
                        apply_core_geometry(xw->core_id);
                    }
                } else {
                    XWindowChanges xc;
                    xc.x = ev.xconfigurerequest.x;
                    xc.y = ev.xconfigurerequest.y;
                    xc.width = ev.xconfigurerequest.width;
                    xc.height = ev.xconfigurerequest.height;
                    xc.border_width = ev.xconfigurerequest.border_width;
                    XConfigureWindow(X.dpy, ev.xconfigurerequest.window,
                                     ev.xconfigurerequest.value_mask, &xc);
                }
                break;
            }
            case DestroyNotify: {
                XWin *xw = xwin_by_client(ev.xdestroywindow.window);
                if (xw) unmanage(xw, true);
                break;
            }
            case UnmapNotify: {
                XWin *xw = xwin_by_client(ev.xunmap.window);
                if (xw && ev.xunmap.window != X.taskbar) {
                    WmWindow *cw = wm_find(&g_wm, xw->core_id);
                    if (cw && !cw->minimized) {
                        /* client unmapped itself: hide frame, keep task entry */
                        cw->minimized = true;
                        XUnmapWindow(X.dpy, xw->frame);
                        XClearArea(X.dpy, X.taskbar, 0, 0, 0, 0, True);
                    }
                }
                break;
            }
            case Expose: {
                if (ev.xexpose.count == 0) {
                    XWin *xw = xwin_by_frame(ev.xexpose.window);
                    if (xw) draw_frame(xw);
                    else if (ev.xexpose.window == X.taskbar) draw_taskbar();
                }
                break;
            }
            case ButtonPress: {
                if (ev.xbutton.window == X.taskbar) {
                    taskbar_click(ev.xbutton.x, ev.xbutton.y);
                    break;
                }
                XWin *xw = xwin_by_frame(ev.xbutton.window);
                if (!xw) break;
                WmWindow *cw = wm_find(&g_wm, xw->core_id);
                if (!cw) break;
                wm_focus(&g_wm, xw->core_id);
                WmHit hit = wm_hit_test(cw, ev.xbutton.x, ev.xbutton.y);
                switch (hit) {
                case WM_HIT_CLOSE:
                    wm_hook_close_window(xw->core_id);
                    break;
                case WM_HIT_MIN:
                    wm_toggle_minimize(&g_wm, xw->core_id);
                    if (cw->minimized) XUnmapWindow(X.dpy, xw->frame);
                    else { XMapWindow(X.dpy, xw->frame); wm_focus(&g_wm, xw->core_id); }
                    XClearArea(X.dpy, X.taskbar, 0, 0, 0, 0, True);
                    break;
                case WM_HIT_MAX:
                    wm_toggle_maximize(&g_wm, xw->core_id);
                    apply_core_geometry(xw->core_id);
                    XClearWindow(X.dpy, xw->frame);
                    break;
                case WM_HIT_TITLE:
                case WM_HIT_EDGE_L: case WM_HIT_EDGE_R:
                case WM_HIT_EDGE_T: case WM_HIT_EDGE_B:
                case WM_HIT_CORNER_TL: case WM_HIT_CORNER_TR:
                case WM_HIT_CORNER_BL: case WM_HIT_CORNER_BR:
                    drag_loop(xw->core_id, hit);
                    XClearWindow(X.dpy, xw->frame);
                    break;
                default: break;
                }
                break;
            }
            case KeyPress: {
                KeySym ks = XLookupKeysym(&ev.xkey, 0);
                if (ks == XK_Tab && (ev.xkey.state & Mod1Mask)) {
                    wm_cycle_focus(&g_wm);
                }
                break;
            }
            default: break;
            }
        }

        /* idle: plugin ticks + 1 Hz taskbar refresh (clock/date) */
        plugins_call(&g_plugins, "on_tick", "");
        time_t now = time(NULL);
        if (now != last_sec) {
            last_sec = now;
            if (X.taskbar) XClearArea(X.dpy, X.taskbar, 0, 0, 0, 0, True);
        }
        usleep(30000);   /* ~30 fps idle cadence */
    }

    /* unreachable in practice: the WM runs until X session ends */
    plugins_call(&g_plugins, "on_stop", "");
    XCloseDisplay(X.dpy);
    return 0;
}
