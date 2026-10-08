#ifndef W3M_X11_H
#define W3M_X11_H

/* X11-specific state for the W3M window manager frontend.
   The logical WM core (wm.c) is backend-agnostic; this header ties the
   core's window model to real X11 windows. */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdbool.h>

#define W3M_FRAME_EVENT_BASE (SubstructureRedirectMask)

typedef struct XWindow {
    Window client;      /* the real application window */
    Window frame;       /* our decoration frame (parent of client) */
    Window titlebar;    /* title bar + buttons */
    int core_id;        /* id in the logical WmState */
    GC gc;
} XWindow;

typedef struct XState {
    Display *dpy;
    Window root;
    int screen;
    int screen_w, screen_h;
    XWindow xwins[32];
    int count;
    Window taskbar;
    GC taskbar_gc;
    bool taskbar_mapped;
    /* drag state */
    int drag_core_id;
    int drag_hit;
    int drag_start_x, drag_start_y;
    int drag_orig_x, drag_orig_y, drag_orig_w, drag_orig_h;
    /* take-focus cycling */
    int cycling;
} XState;

XWindow *xstate_find_by_client(XState *xs, Window w);
XWindow *xstate_find_by_frame(XState *xs, Window w);
XWindow *xstate_find_by_titlebar(XState *xs, Window w);
XWindow *xstate_find_by_core(XState *xs, int core_id);

bool xwin_create(XState *xs, int core_id);
void xwin_destroy(XState *xs, XWindow *xw);
void xwin_sync_geometry(XState *xs, struct WmState *wm);  /* fwd decl via wm.h in caller */

#endif
