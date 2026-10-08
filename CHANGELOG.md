# Changelog

## [0.4.0] — Reparenting completo: W3M es un WM usable

### Añadido
- **Reparenting**: cada cliente se adopta en un frame Win 3.x propio
  (bisel, barra de título, botones `^` `_` `x`), gestionado por `MapRequest`.
- **Mover y redimensionar** con pointer grab: título para mover,
  bordes/esquinas para redimensionar (semántica del core `wm.c`).
- **Taskbar X11**: botones por ventana (clic = enfocar/restaurar/minimizar),
  panel configurable con módulos nativos (clock/date) y slots de plugins.
- **Alt+Tab** (XGrabKey), maximizar/minimizar/cerrar desde los botones.
- Cierre cooperativo: `WM_DELETE_WINDOW` si el cliente lo soporta,
  `XDestroyWindow` como último recurso.
- `ConfigureRequest` respetado (apps que piden tamaño crecen el frame).
- Cleanup correcto en `DestroyNotify`/`UnmapNotify`.
- Stub Xlib (`tests/x11_stub.h`) para verificación sin X11 headers.

## [0.3.0] — Primer esqueleto

### Añadido
- Core lógico reutilizado de win3wm (35 tests), config, host Lua,
  addons, PKGBUILD, docs.
