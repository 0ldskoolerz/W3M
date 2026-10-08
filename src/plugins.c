#include "plugins.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if defined(WM_WITH_LUA) && defined(__has_include)
#if __has_include(<lua.h>)
#define WM_HAVE_LUA 1
#endif
#elif defined(WM_WITH_LUA)
#define WM_HAVE_LUA 1
#endif

#if defined(WM_HAVE_LUA)
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#endif

void plugins_init(PluginHost *host) {
    memset(host, 0, sizeof *host);
#if defined(WM_HAVE_LUA)
    host->lua_available = true;
#endif
}

#if defined(WM_HAVE_LUA)

static int push_args(lua_State *L, const char *fmt, va_list ap) {
    int n = 0;
    for (const char *p = fmt; *p; p++) {
        switch (*p) {
        case 'd': lua_pushinteger(L, va_arg(ap, int)); n++; break;
        case 'f': lua_pushnumber(L, va_arg(ap, double)); n++; break;
        case 's': lua_pushstring(L, va_arg(ap, const char *)); n++; break;
        }
    }
    return n;
}

/* Plugin Lua API, provided to every script:
     wm.name                    -> string (plugin identity)
     wm.log(msg)                -> print to stdout
     wm.notify(msg)             -> show in the WM status line
     wm.create_window(title,w,h)-> window id
     wm.close_window(id)        -> bool
     wm.focus(id)
     wm.window_title(id)        -> string
     wm.set_title(id, title)
     wm.focus_next()            -> alt-tab behavior
     (see plugins/README.md)                                   */

static PluginHost *g_host;
static void *g_wm; /* WmState*, opaque */

static int api_log(lua_State *L) {
    printf("[plugin] %s\n", luaL_checkstring(L, 1));
    return 0;
}
static int api_notify(lua_State *L);
static int api_status(lua_State *L);
static int api_create_window(lua_State *L);
static int api_close_window(lua_State *L);
static int api_focus(lua_State *L);
static int api_window_title(lua_State *L);
static int api_set_title(lua_State *L);
static int api_focus_next(lua_State *L);
static int api_window_count(lua_State *L);
static int api_window_ids(lua_State *L);

/* implemented in main.c, weak so tests can link without them */
__attribute__((weak)) void wm_hook_notify(const char *msg) { (void)msg; }
__attribute__((weak)) void wm_hook_status(const char *module, const char *text) { (void)module; (void)text; }
__attribute__((weak)) int  wm_hook_create_window(const char *t, int w, int h) { (void)t; (void)w; (void)h; return -1; }
__attribute__((weak)) bool wm_hook_close_window(int id) { (void)id; return false; }
__attribute__((weak)) void wm_hook_focus(int id) { (void)id; }
__attribute__((weak)) const char *wm_hook_window_title(int id) { (void)id; return ""; }
__attribute__((weak)) void wm_hook_set_title(int id, const char *t) { (void)id; (void)t; }
__attribute__((weak)) void wm_hook_focus_next(void) {}
__attribute__((weak)) int  wm_hook_window_count(void) { return 0; }
__attribute__((weak)) int  wm_hook_window_id(int idx) { (void)idx; return -1; }

static int api_notify(lua_State *L) { wm_hook_notify(luaL_checkstring(L, 1)); return 0; }
static int api_status(lua_State *L) {
    wm_hook_status(luaL_checkstring(L, 1), luaL_checkstring(L, 2));
    return 0;
}
static int api_create_window(lua_State *L) {
    const char *t = luaL_checkstring(L, 1);
    int w = (int)luaL_checkinteger(L, 2);
    int h = (int)luaL_checkinteger(L, 3);
    lua_pushinteger(L, wm_hook_create_window(t, w, h));
    return 1;
}
static int api_close_window(lua_State *L) {
    lua_pushboolean(L, wm_hook_close_window((int)luaL_checkinteger(L, 1)));
    return 1;
}
static int api_focus(lua_State *L) { wm_hook_focus((int)luaL_checkinteger(L, 1)); return 0; }
static int api_window_title(lua_State *L) {
    lua_pushstring(L, wm_hook_window_title((int)luaL_checkinteger(L, 1)));
    return 1;
}
static int api_set_title(lua_State *L) {
    wm_hook_set_title((int)luaL_checkinteger(L, 1), luaL_checkstring(L, 2));
    return 0;
}
static int api_focus_next(lua_State *L) { wm_hook_focus_next(); return 0; }
static int api_window_count(lua_State *L) {
    lua_pushinteger(L, wm_hook_window_count());
    return 1;
}
static int api_window_ids(lua_State *L) {
    int n = wm_hook_window_count();
    lua_newtable(L);
    for (int i = 0; i < n; i++) {
        lua_pushinteger(L, i + 1);
        lua_pushinteger(L, wm_hook_window_id(i));
        lua_settable(L, -3);
    }
    return 1;
}

bool plugins_load(PluginHost *host, const char *name) {
    if (host->count >= PLUG_MAX) return false;
    for (int i = 0; i < host->count; i++)
        if (!strcmp(host->plugins[i].name, name)) return true;
    char path[256];
    snprintf(path, sizeof path, "plugins/%s.lua", name);
    lua_State *L = luaL_newstate();
    if (!L) return false;
    luaL_openlibs(L);
    lua_newtable(L);
    lua_pushstring(L, name); lua_setfield(L, -2, "name");
    lua_pushcfunction(L, api_log); lua_setfield(L, -2, "log");
    lua_pushcfunction(L, api_notify); lua_setfield(L, -2, "notify");
    lua_pushcfunction(L, api_status); lua_setfield(L, -2, "status");
    lua_pushcfunction(L, api_create_window); lua_setfield(L, -2, "create_window");
    lua_pushcfunction(L, api_close_window); lua_setfield(L, -2, "close_window");
    lua_pushcfunction(L, api_focus); lua_setfield(L, -2, "focus");
    lua_pushcfunction(L, api_window_title); lua_setfield(L, -2, "window_title");
    lua_pushcfunction(L, api_set_title); lua_setfield(L, -2, "set_title");
    lua_pushcfunction(L, api_focus_next); lua_setfield(L, -2, "focus_next");
    lua_pushcfunction(L, api_window_count); lua_setfield(L, -2, "window_count");
    lua_pushcfunction(L, api_window_ids); lua_setfield(L, -2, "window_ids");
    lua_setglobal(L, "wm");
    if (luaL_dofile(L, path) != LUA_OK) {
        fprintf(stderr, "[plugins] %s: %s\n", name, lua_tostring(L, -1));
        lua_close(L);
        return false;
    }
    Plugin *p = &host->plugins[host->count++];
    snprintf(p->name, PLUG_MAXNAME, "%s", name);
    p->L = L;
    p->ok = true;
    return true;
}

void plugins_unload(PluginHost *host, const char *name) {
    for (int i = 0; i < host->count; i++) {
        if (!strcmp(host->plugins[i].name, name)) {
            if (host->plugins[i].L) lua_close((lua_State *)host->plugins[i].L);
            memmove(&host->plugins[i], &host->plugins[i + 1],
                    (size_t)(host->count - i - 1) * sizeof(Plugin));
            host->count--;
            return;
        }
    }
}

bool plugins_call(PluginHost *host, const char *fn, const char *fmt, ...) {
    bool any = false;
    for (int i = 0; i < host->count; i++) {
        lua_State *L = host->plugins[i].L;
        if (!L) continue;
        lua_getglobal(L, fn);
        if (!lua_isfunction(L, -1)) { lua_pop(L, 1); continue; }
        va_list ap;
        va_start(ap, fmt);
        int n = push_args(L, fmt, ap);
        va_end(ap);
        if (lua_pcall(L, n, 0, 0) != LUA_OK) {
            fprintf(stderr, "[plugins] %s.%s: %s\n", host->plugins[i].name, fn,
                    lua_tostring(L, -1));
            lua_pop(L, 1);
        } else any = true;
    }
    return any;
}

#else /* no Lua support */

bool plugins_load(PluginHost *host, const char *name) {
    fprintf(stderr, "[plugins] '%s' skipped: built without Lua (-DWM_WITH_LUA)\n", name);
    (void)host;
    return false;
}
void plugins_unload(PluginHost *host, const char *name) { (void)host; (void)name; }
bool plugins_call(PluginHost *host, const char *fn, const char *fmt, ...) {
    (void)host; (void)fn; (void)fmt;
    return false;
}

#endif
