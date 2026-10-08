#ifndef WM_H
#define WM_H

#include <stdbool.h>

#define WM_MAX_WINDOWS 32
#define WM_MAX_TITLE  64
#define WM_TASKBAR_H  24

typedef struct WmRect { int x, y, w, h; } WmRect;

typedef enum {
    WM_HIT_NONE = 0,
    WM_HIT_TITLE,
    WM_HIT_CLOSE,
    WM_HIT_MIN,
    WM_HIT_MAX,
    WM_HIT_BODY,
    WM_HIT_EDGE_L, WM_HIT_EDGE_R, WM_HIT_EDGE_T, WM_HIT_EDGE_B,
    WM_HIT_CORNER_TL, WM_HIT_CORNER_TR, WM_HIT_CORNER_BL, WM_HIT_CORNER_BR
} WmHit;

typedef struct WmWindow {
    int id;
    char title[WM_MAX_TITLE];
    WmRect normal;     /* geometry when restored */
    WmRect box;        /* current geometry (screen coords) */
    bool minimized;
    bool maximized;
    int z;             /* higher = on top */
    bool focused;
} WmWindow;

typedef struct WmState {
    WmWindow windows[WM_MAX_WINDOWS];
    int count;
    int next_id;
    int focus_id;             /* id of focused window, -1 if none */
    int screen_w, screen_h;  /* desktop area (excludes taskbar) */
} WmState;

void        wm_init(WmState *wm, int screen_w, int screen_h);
int         wm_create_window(WmState *wm, const char *title, int w, int h);
bool        wm_close_window(WmState *wm, int id);
WmWindow   *wm_find(WmState *wm, int id);
WmWindow   *wm_window_at(WmState *wm, int x, int y);       /* topmost visible */
void        wm_focus(WmState *wm, int id);
void        wm_cycle_focus(WmState *wm);                    /* alt-tab */
WmWindow   *wm_focused(WmState *wm);
void        wm_move_window(WmState *wm, int id, int dx, int dy);
void        wm_resize_window(WmState *wm, int id, int edge, int dx, int dy);
void        wm_toggle_minimize(WmState *wm, int id);
void        wm_toggle_maximize(WmState *wm, int id);
void        wm_restore(WmState *wm, int id);
WmHit       wm_hit_test(const WmWindow *win, int x, int y);
void        wm_constrain(WmState *wm);
const char *wm_hit_name(WmHit hit);

#endif
