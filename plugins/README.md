# Plugins de W3M

Un plugin es un archivo `plugins/<nombre>.lua`. Se carga automáticamente si
figura en `plugins = ...` del archivo de configuración. No requiere
recompilar nada: edita el script y reinicia W3M.

## API global `wm`

| Función                        | Descripción                          |
|--------------------------------|--------------------------------------|
| `wm.name`                      | string con el nombre del plugin      |
| `wm.log(msg)`                  | imprime en stdout                    |
| `wm.notify(msg)`               | muestra msg en la línea de estado    |
| `wm.create_window(t, w, h)`    | crea ventana, devuelve su id (o -1)  |
| `wm.close_window(id)`          | cierra la ventana id, devuelve bool  |
| `wm.focus(id)`                 | enfoca la ventana id                 |
| `wm.window_title(id)`          | devuelve el título de la ventana     |
| `wm.set_title(id, titulo)`     | cambia el título                     |
| `wm.focus_next()`              | equivalente a Alt+Tab                |
| `wm.status(module, text)`       | escribe text en una sección del panel |
| `wm.window_count()`             | número de ventanas abiertas          |
| `wm.window_ids()`              | tabla con los ids de las ventanas     |

## Eventos

El core invoca estas funciones de tu script si existen:

| Evento                    | Argumentos        | Cuándo                    |
|---------------------------|-------------------|---------------------------|
| `on_start()`              | –                 | al arrancar, tras cargar   |
| `on_tick()`               | –                 | cada frame (~60/s)         |
| `on_stop()`               | –                 | al cerrar W3M           |
| `on_window_focused(id, t)`| id, título        | al enfocar una ventana     |
| `on_window_closed(id)`     | id                | al cerrar una ventana      |

## Panel de estado configurable

En `W3M.conf`, `panel_modules` define las secciones del panel, en orden.
`tasks`, `clock` y `date` son nativas del core. Cualquier otro nombre es un
slot que tu plugin llena con `wm.status()`:

```
panel_modules = tasks,clock,date,mem,apps
```

```lua
-- un plugin puede añadir su propia sección:
wm.status("mem", "RAM 512/2048M")
```

## Ejemplo mínimo

```lua
function on_start()
    wm.notify("hola!")
end

function on_window_focused(id, title)
    wm.log("foco en " .. title)
end
```

## Limitaciones actuales (MVP)

- Los errores de Lua se reportan en stderr y el plugin se salta; no tira el WM.
- No hay hotkeys expuestos a Lua todavía (planeado: `wm.on_key`).
- `launcher.lua` incluye `open_demo()` listo para conectar a un hotkey.
