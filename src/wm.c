#include "wm.h"

#include <stdio.h>
#include <string.h>

#define WM_EDGE 6   /* resize border thickness */

static int wm_top_z(const WmState *wm) {
    int z = 0;
    for (int i = 0; i < wm->count; i++)
        if (wm->windows[i].z > z) z = wm->windows[i].z;
    return z;
}

void wm_init(WmState *wm, int screen_w, int screen_h) {
    memset(wm, 0, sizeof *wm);
    wm->count = 0;
    wm->next_id = 1;
    wm->focus_id = -1;
    wm->screen_w = screen_w;
    wm->screen_h = screen_h;
}

int wm_create_window(WmState *wm, const char *title, int w, int h) {
    if (wm->count >= WM_MAX_WINDOWS) return -1;
    WmWindow *win = &wm->windows[wm->count++];
    memset(win, 0, sizeof *win);
    win->id = wm->next_id++;
    snprintf(win->title, WM_MAX_TITLE, "%s", title ? title : "Window");
    win->box.w = w;
    win->box.h = h;
    /* cascade placement, Win 3.x style */
    int n = (win->id - 1) % 8;
    win->box.x = 24 + n * 26;
    win->box.y = 24 + n * 26;
    win->normal = win->box;
    win->z = wm_top_z(wm) + 1;
    win->focused = true;
    wm_focus(wm, win->id);
    return win->id;
}

bool wm_close_window(WmState *wm, int id) {
    for (int i = 0; i < wm->count; i++) {
        if (wm->windows[i].id == id) {
            memmove(&wm->windows[i], &wm->windows[i + 1],
                    (size_t)(wm->count - i - 1) * sizeof(WmWindow));
            wm->count--;
            if (wm->focus_id == id) {
                wm->focus_id = -1;
                if (wm->count > 0) wm_focus(wm, wm->windows[wm->count - 1].id);
            }
            return true;
        }
    }
    return false;
}

WmWindow *wm_find(WmState *wm, int id) {
    for (int i = 0; i < wm->count; i++)
        if (wm->windows[i].id == id) return &wm->windows[i];
    return NULL;
}

WmWindow *wm_window_at(WmState *wm, int x, int y) {
    int best_z = -1;
    WmWindow *best = NULL;
    for (int i = 0; i < wm->count; i++) {
        WmWindow *w = &wm->windows[i];
        if (w->minimized) continue;
        if (x < w->box.x || y < w->box.y ||
            x >= w->box.x + w->box.w || y >= w->box.y + w->box.h) continue;
        if (w->z > best_z) { best_z = w->z; best = w; }
    }
    return best;
}

void wm_focus(WmState *wm, int id) {
    if (!wm_find(wm, id)) return;
    int top = wm_top_z(wm);
    for (int i = 0; i < wm->count; i++)
        wm->windows[i].focused = (wm->windows[i].id == id);
    wm_find(wm, id)->z = top + 1;
    wm->focus_id = id;
}

void wm_cycle_focus(WmState *wm) {
    if (wm->count == 0) return;
    /* pick next visible window after current focus in z order */
    int order[WM_MAX_WINDOWS];
    int n = 0;
    for (int z = 0; z <= wm_top_z(wm); z++)
        for (int i = 0; i < wm->count; i++) {
            WmWindow *w = &wm->windows[i];
            if (w->z == z && !w->minimized) order[n++] = i;
        }
    if (n == 0) return;
    int cur = -1;
    for (int k = 0; k < n; k++)
        if (wm->windows[order[k]].id == wm->focus_id) cur = k;
    wm_focus(wm, wm->windows[order[(cur + 1) % n]].id);
}

WmWindow *wm_focused(WmState *wm) {
    return wm_find(wm, wm->focus_id);
}

void wm_move_window(WmState *wm, int id, int dx, int dy) {
    WmWindow *w = wm_find(wm, id);
    if (!w) return;
    w->box.x += dx;
    w->box.y += dy;
    wm_constrain(wm);
}

void wm_resize_window(WmState *wm, int id, int edge, int dx, int dy) {
    WmWindow *w = wm_find(wm, id);
    if (!w) return;
    switch (edge) {
    case WM_HIT_EDGE_L:  w->box.x += dx; w->box.w -= dx; break;
    case WM_HIT_EDGE_R:  w->box.w += dx; break;
    case WM_HIT_EDGE_T:  w->box.y += dy; w->box.h -= dy; break;
    case WM_HIT_EDGE_B:  w->box.h += dy; break;
    case WM_HIT_CORNER_TL: w->box.x += dx; w->box.w -= dx;
                           w->box.y += dy; w->box.h -= dy; break;
    case WM_HIT_CORNER_TR: w->box.w += dx;
                           w->box.y += dy; w->box.h -= dy; break;
    case WM_HIT_CORNER_BL: w->box.x += dx; w->box.w -= dx;
                           w->box.h += dy; break;
    case WM_HIT_CORNER_BR: w->box.w += dx; w->box.h += dy; break;
    }
    wm_constrain(wm);
}

void wm_toggle_minimize(WmState *wm, int id) {
    WmWindow *w = wm_find(wm, id);
    if (!w) return;
    w->minimized = !w->minimized;
    if (!w->minimized) wm_focus(wm, id);
    else if (wm->focus_id == id) {
        wm->focus_id = -1;
        for (int i = 0; i < wm->count; i++)
            wm->windows[i].focused = false;
    }
}

void wm_toggle_maximize(WmState *wm, int id) {
    WmWindow *w = wm_find(wm, id);
    if (!w) return;
    if (w->maximized) {
        w->box = w->normal;
        w->maximized = false;
    } else {
        w->normal = w->box;
        w->box.x = 0; w->box.y = 0;
        w->box.w = wm->screen_w;
        w->box.h = wm->screen_h;
        w->maximized = true;
    }
    wm_focus(wm, id);
}

void wm_restore(WmState *wm, int id) {
    WmWindow *w = wm_find(wm, id);
    if (!w) return;
    if (w->maximized) wm_toggle_maximize(wm, id);
    if (w->minimized) wm_toggle_minimize(wm, id);
}

void wm_constrain(WmState *wm) {
    for (int i = 0; i < wm->count; i++) {
        WmWindow *w = &wm->windows[i];
        if (w->box.w < 80) w->box.w = 80;
        if (w->box.h < 40) w->box.h = 40;
        if (w->box.x < -w->box.w + WM_EDGE) w->box.x = -w->box.w + WM_EDGE;
        if (w->box.y < 0) w->box.y = 0;
        if (w->box.x > wm->screen_w - WM_EDGE) w->box.x = wm->screen_w - WM_EDGE;
        if (w->box.y > wm->screen_h - WM_EDGE) w->box.y = wm->screen_h - WM_EDGE;
    }
}

WmHit wm_hit_test(const WmWindow *win, int x, int y) {
    if (x < win->box.x || y < win->box.y ||
        x >= win->box.x + win->box.w || y >= win->box.y + win->box.h)
        return WM_HIT_NONE;
    int rel_x = x - win->box.x;
    int rel_y = y - win->box.y;
    int right = win->box.w - 1;
    if (rel_y >= 18) {   /* below title bar */
        if (rel_x < WM_EDGE) return WM_HIT_EDGE_L;
        if (rel_x >= win->box.w - WM_EDGE) return WM_HIT_EDGE_R;
        if (rel_y >= win->box.h - WM_EDGE) return WM_HIT_EDGE_B;
        return WM_HIT_BODY;
    }
    /* title bar row: buttons on the right, Win 3.x style */
    if (rel_x >= right - 14 && rel_x <= right && rel_y <= 17) return WM_HIT_CLOSE;
    if (rel_x >= right - 30 && rel_x < right - 14 && rel_y <= 17) return WM_HIT_MIN;
    if (rel_x >= right - 46 && rel_x < right - 30 && rel_y <= 17) return WM_HIT_MAX;
    return WM_HIT_TITLE;
}

const char *wm_hit_name(WmHit hit) {
    switch (hit) {
    case WM_HIT_NONE: return "none";
    case WM_HIT_TITLE: return "title";
    case WM_HIT_CLOSE: return "close";
    case WM_HIT_MIN: return "min";
    case WM_HIT_MAX: return "max";
    case WM_HIT_BODY: return "body";
    case WM_HIT_EDGE_L: return "edge_l";
    case WM_HIT_EDGE_R: return "edge_r";
    case WM_HIT_EDGE_T: return "edge_t";
    case WM_HIT_EDGE_B: return "edge_b";
    case WM_HIT_CORNER_TL: return "corner_tl";
    case WM_HIT_CORNER_TR: return "corner_tr";
    case WM_HIT_CORNER_BL: return "corner_bl";
    case WM_HIT_CORNER_BR: return "corner_br";
    }
    return "?";
}
