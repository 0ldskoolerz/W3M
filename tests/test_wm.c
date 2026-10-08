#include "../src/wm.h"
#include "../src/config.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int passed = 0;
#define CHECK(cond) do { \
    if (cond) { passed++; } \
    else { fprintf(stderr, "FALLO: %s (linea %d)\n", #cond, __LINE__); return 1; } \
} while (0)

int main(void) {
    WmState wm;
    wm_init(&wm, 800, 600 - WM_TASKBAR_H);

    CHECK(wm.count == 0 && wm.focus_id == -1);

    int a = wm_create_window(&wm, "Alpha", 200, 100);
    int b = wm_create_window(&wm, "Beta", 200, 100);
    int c = wm_create_window(&wm, "Gamma", 200, 100);
    CHECK(a > 0 && b > a && c > b);
    CHECK(wm.count == 3);

    /* newest window focused and on top */
    CHECK(wm.focus_id == c);
    CHECK(wm_find(&wm, c)->z > wm_find(&wm, a)->z);
    CHECK(wm_focused(&wm)->id == c);

    /* focus switching raises z */
    wm_focus(&wm, a);
    CHECK(wm_focused(&wm)->id == a);
    CHECK(wm_find(&wm, a)->z > wm_find(&wm, c)->z);

    /* alt-tab cycles through visible windows */
    wm_cycle_focus(&wm);
    CHECK(wm_focused(&wm)->id != a);
    wm_cycle_focus(&wm);
    wm_cycle_focus(&wm);
    CHECK(wm_focused(&wm)->id == a);

    /* hit testing: title bar, buttons, body */
    WmWindow *w = wm_find(&wm, a);
    w->box.x = 50; w->box.y = 50; w->box.w = 200; w->box.h = 100;
    CHECK(wm_hit_test(w, 60, 60) == WM_HIT_TITLE);
    CHECK(wm_hit_test(w, 235, 60) == WM_HIT_CLOSE);
    CHECK(wm_hit_test(w, 220, 60) == WM_HIT_MIN);
    CHECK(wm_hit_test(w, 205, 60) == WM_HIT_MAX);
    CHECK(wm_hit_test(w, 60, 90) == WM_HIT_BODY);
    CHECK(wm_hit_test(w, 52, 90) == WM_HIT_EDGE_L);
    CHECK(wm_hit_test(w, 10, 10) == WM_HIT_NONE);

    /* minimize unfocuses; restore refocuses */
    wm_toggle_minimize(&wm, a);
    CHECK(wm_find(&wm, a)->minimized);
    CHECK(wm_focused(&wm) == NULL || wm_focused(&wm)->id != a);
    wm_toggle_minimize(&wm, a);
    CHECK(!wm_find(&wm, a)->minimized && wm.focus_id == a);

    /* maximize fills screen, restore keeps old geometry */
    int ox = w->box.x, oy = w->box.y;
    wm_toggle_maximize(&wm, a);
    CHECK(w->box.w == 800 && w->box.h == 600 - WM_TASKBAR_H);
    wm_toggle_maximize(&wm, a);
    CHECK(w->box.x == ox && w->box.y == oy);

    /* move + constrain inside screen */
    wm_move_window(&wm, a, -500, -500);
    CHECK(w->box.x >= -w->box.w + 6 && w->box.y >= 0);

    /* resize from bottom-right corner */
    wm_resize_window(&wm, a, WM_HIT_CORNER_BR, 1000, 1000);
    CHECK(w->box.w >= 80 && w->box.h >= 40);

    /* close removes and re-focuses */
    CHECK(wm_close_window(&wm, b));
    CHECK(wm.count == 2 && wm_find(&wm, b) == NULL);
    CHECK(!wm_close_window(&wm, b));

    /* window_at picks topmost */
    WmWindow *top = wm_window_at(&wm, w->box.x + 10, w->box.y + 10);
    CHECK(top != NULL && top->id == w->id);

    /* config */
    WmConfig cfg;
    CHECK(cfg_load(&cfg, "config/w3m.conf"));
    CHECK(cfg.screen_w == 1024 && cfg.screen_h == 768);
    CHECK(cfg.show_taskbar);
    CHECK(!strcmp(cfg.plugins, "clock,greeter,mem,audio,red,bluetooth,discos"));
    CHECK(!strcmp(cfg.panel_modules, "tasks,audio,red,bt,discos,mem,clock,date"));
    CHECK(cfg_get(&cfg, "wallpaper") != NULL && !strcmp(cfg_get(&cfg, "wallpaper"), "darkslategrey"));
    CHECK(cfg_get(&cfg, "no_existe") == NULL);

    printf("OK: %d checks pasados\n", passed);
    return 0;
}
