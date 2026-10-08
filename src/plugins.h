#ifndef WM_PLUGINS_H
#define WM_PLUGINS_H

#include <stdbool.h>

#define PLUG_MAX     16
#define PLUG_MAXNAME 32

typedef struct Plugin {
    char name[PLUG_MAXNAME];
    void *L;          /* lua_State*, opaque so the core compiles without Lua */
    bool ok;
} Plugin;

typedef struct PluginHost {
    Plugin plugins[PLUG_MAX];
    int count;
    bool lua_available;   /* false when built without Lua support */
} PluginHost;

void plugins_init(PluginHost *host);
bool plugins_load(PluginHost *host, const char *name);
void plugins_unload(PluginHost *host, const char *name);
bool plugins_call(PluginHost *host, const char *fn, const char *fmt, ...); /* pass "" for no args */
/* fmt: d=int, f=double, s=string; returns false if no plugin handled it */

#endif
