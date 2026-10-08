# W3M — Guía de plugins (Lua)

Los plugins convierten W3M en un escritorio extensible sin recompilar:
son scripts Lua que el core carga al arrancar y a los que expone la API
global `wm.*`.

---

## 1. Anatomía de un plugin

Un plugin es un archivo `plugins/<nombre>.lua`. Se activa listándolo en
la configuración:

```ini
plugins = clock,greeter,mem,apps
```

Cada plugin se ejecuta en su propio `lua_State` aislado: un error en uno
no tira el WM ni afecta a los demás. Si un script falla al cargar, W3M
lo reporta en stderr y continúa sin él.

### Plantilla mínima

```lua
-- plugins/miplugin.lua
function on_start()
    wm.log("miplugin cargado")
    wm.notify("¡Hola!")
end
```

Guarda el archivo, añade `miplugin` a `plugins = ...` en el .conf,
reinicia W3M. No hay que recompilar nada.

---

## 2. API `wm.*` (referencia completa)

### Mensajes y panel

| Función | Descripción |
|---|---|
| `wm.name` | (string) nombre del plugin, el de la config |
| `wm.log(msg)` | imprime `[plugin] msg` en stdout |
| `wm.notify(msg)` | muestra `msg` en la línea de estado (izquierda del panel) |
| `wm.status(modulo, texto)` | escribe `texto` en la sección `modulo` del panel |

`wm.status` es la pieza clave del panel configurable: crea la sección si
no existe y actualiza su texto en el próximo frame.

### Ventanas

| Función | Descripción |
|---|---|
| `wm.create_window(titulo, w, h)` | crea una ventana; devuelve su id o `-1` |
| `wm.close_window(id)` | cierra; devuelve `true` si existía |
| `wm.focus(id)` | enfoca y eleva la ventana |
| `wm.focus_next()` | cicla el foco (equivalente a Alt+Tab) |
| `wm.window_title(id)` | devuelve el título de la ventana |
| `wm.set_title(id, titulo)` | renombra la ventana |
| `wm.window_count()` | número de ventanas abiertas |
| `wm.window_ids()` | tabla Lua `{1=id1, 2=id2, ...}` con todos los ids |

### Constantes

| Valor | Descripción |
|---|---|
| `wm.name` | nombre del plugin tal como figura en la config |

---

## 3. Eventos que el core invoca

Declara estas funciones globales en tu script; el core las llama si
existen (todas opcionales):

| Evento | Firma | Cuándo se dispara |
|---|---|---|
| `on_start()` | `()` | una vez, tras cargar todos los plugins |
| `on_tick()` | `()` | cada frame (~60 veces/seg) |
| `on_stop()` | `()` | al cerrar W3M |
| `on_window_focused(id, titulo)` | `(int, string)` | al enfocar una ventana |
| `on_window_closed(id)` | `(int)` | al cerrar una ventana |
| `on_key(code, nombre)` | `(int, string)` | al pulsar cualquier tecla |

### `on_key` — hotkeys desde Lua

Cada pulsación de tecla llega a todos los plugins con dos argumentos:
el código Xlib/X11 (`code`) y un nombre legible (`nombre`). Nombres que envía
el core: `F1`..`F12`, caracteres imprimibles (`a`, `1`, ` `, ...),
`ESC`, `ENTER`, `TAB`, `BACKSPACE`, `UP`, `DOWN`, `LEFT`, `RIGHT`,
`LALT`, `RALT`, `LCTRL`.

```lua
function on_key(code, name)
    if name == "F2" then wm.create_window("Nueva", 220, 120) end
    if name == "q"  then wm.notify("pulsaste q") end
end
```

⚠️ Las combinaciones con modificadores (Alt+Tab, F1, Esc) también llegan a
`on_key`; el core las procesa además con su comportamiento propio.

⚠️ `on_tick` corre a 60 Hz: si lees archivos (`/proc`, etc.) o haces
trabajo pesado, **cachea con un timer** como hace `mem.lua` (cada 5 s).

---

## 4. El panel de estado configurable (estilo lxpanel)

El panel es una lista de secciones definida en el .conf:

```ini
panel_modules = tasks,clock,date,mem,apps
```

- **Secciones nativas del core**: `tasks` (botones de ventanas corriendo),
  `clock` (HH:MM), `date` (dd/mm/aaaa).
- **Cualquier otro nombre** es un slot libre: lo llena cualquier plugin
  con `wm.status("ese_nombre", "texto")`.

Los slots se dibujan apilados de derecha a izquierda en el orden inverso
en que aparecen en la lista. Ejemplo de sección propia:

```lua
function on_tick()
    wm.status("uptime", "up " .. math.floor(os.clock()) .. "s")
end
```

Con `panel_modules = tasks,clock,uptime` verías: `[ventanas] [up 123s] [HH:MM]`

---

## 5. Ejemplos incluidos

| Plugin | Qué demuestra |
|---|---|
| `keys.lua` | evento `on_key`: muestra la última tecla pulsada en el panel |
| `clock.lua` | `wm.status` para hora y fecha (con cacheo por segundo) |
| `mem.lua` | leer `/proc/meminfo`, formatear, actualizar cada 5 s |
| `apps.lua` | `wm.window_ids()` + `wm.window_title()` para listar apps |
| `greeter.lua` | eventos `on_start`, `on_window_focused`, `on_window_closed` |
| `launcher.lua` | `wm.create_window` desde Lua ( función `open_demo()` lista para conectar) |
| `audio.lua` | volumen/mute reales vía `pactl` (PulseAudio/PipeWire); F8 mute, F9/F10 volumen |
| `red.lua` | conexión de red vía `nmcli`; F11 lista redes wifi |
| `bluetooth.lua` | estado Bluetooth vía `bluetoothctl`; F7 escanea dispositivos |
| `discos.lua` | uso de discos vía `df`/`lsblk`; F5 lista montables, F6 desmonta /media y /mnt |

### Hotkeys de los addons (requieren las herramientas del sistema)

| Tecla | Acción | Requiere |
|---|---|---|
| F5 | listar discos/particiones montables | `lsblk` (util-linux) |
| F6 | desmontar lo montado bajo /media y /mnt | `umount`/`udisks2` |
| F7 | escanear dispositivos Bluetooth | `bluez-utils` |
| F8 | silenciar/activar audio | `pactl` (pipewire-pulse/pulseaudio) |
| F9 / F10 | volumen −5% / +5% | `pactl` |
| F11 | listar redes wifi | `networkmanager` |

Estos addons ejecutan comandos del sistema con los permisos de tu usuario;
en un equipo de escritorio normal funcionan sin más. Lee el código de cada
plugin antes de activarlo (son ~60 líneas cada uno).

---

## 6. Plugin desde cero, paso a paso

**Ejemplo: monitor de batería** (para laptops con `/sys/class/power_supply`):

```lua
-- plugins/battery.lua
local last = 0

function on_tick()
    local now = os.time()
    if now - last < 10 then return end   -- cada 10 s
    last = now
    local f = io.open("/sys/class/power_supply/BAT0/capacity", "r")
    if not f then
        wm.status("battery", "sin BAT0")
        return
    end
    local pct = f:read("*n")
    f:close()
    wm.status("battery", "BAT " .. pct .. "%")
end
```

1. Guarda como `plugins/battery.lua`
2. En `W3M.conf`: `plugins = ...,battery` y añade `battery` a
   `panel_modules`
3. Reinicia W3M

---

## 7. Depuración

- `wm.log("...")` imprime en la consola donde lanzaste el binario:
  úsalo para ver valores.
- Errores de sintaxis: el plugin no carga y stderr muestra
  `[plugins] <nombre>: <error lua>`.
- Errores en runtime (dentro de `on_tick`): se imprimen una vez por
  llamada fallida; el plugin sigue cargado.
- Comenta el plugin en `plugins = ...` para aislar fallos.

## 8. Limitaciones actuales (MVP)

- Sin recarga en caliente: reiniciar W3M para aplicar cambios en un
  script (planeado: hotkey de reload).
- Sin hotkeys expuestos a Lua todavía (`on_key` planeado).
- Cada plugin corre en su propio Lua state: no comparten variables
  globales entre sí (comunícalos vía `wm.status`/`wm.notify`).
- Los scripts acceden al filesystem con los permisos de tu usuario:
  cuida qué lee/escribe un plugin de terceros antes de activarlo.
