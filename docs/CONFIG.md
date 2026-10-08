# W3M — Referencia de configuración

W3M se configura con un único archivo tipo rc-file:

```
config/W3M.conf
```

Puedes pasar otro archivo al arrancar: `./W3M mi_config.conf`

---

## 1. Formato

- Una directiva por línea: `clave = valor`
- `#` inicia un comentario (hasta fin de línea)
- Espacios alrededor de `=` se ignoran
- Strings no llevan comillas: `wallpaper = teal`
- Las claves desconocidas se ignoran sin error (compatibilidad futura)
- Archivo ausente → W3M arranca con valores por defecto

---

## 2. Todas las claves

### Pantalla

| Clave | Tipo | Defecto | Descripción |
|---|---|---|---|
| `screen_w` | int | `1024` | Ancho de la ventana del escritorio |
| `screen_h` | int | `768` | Alto de la ventana del escritorio |
| `titlebar_h` | int | `18` | Alto de la barra de título (px) |
| `taskbar_h` | int | `24` | Alto de la barra de tareas (px) |
| `show_taskbar` | bool | `true` | `true`/`false` — muestra la barra de tareas |

⚠️ Nota MVP: `titlebar_h` afecta al dibujado; el hit-testing del core
asume 18 px. Mantenlo en 18 salvo que ajustes también el core.

### Colores y tema

| Clave | Tipo | Defecto | Descripción |
|---|---|---|---|
| `wallpaper` | color | `teal` | Color del fondo del escritorio |
| `theme_fg` | color | `black` | Texto y líneas (foreground) |
| `theme_bg` | color | `silver` | Caras de ventanas y barra (Win 3.x silver) |
| `theme_accent` | color | `navy` | Barra de título de la ventana enfocada |

**Colores disponibles por nombre:**
`teal`, `silver`, `navy`, `black`, `white`, `gray`, `blue`, `red`, `green`.

Nombre desconocido → se usa el defecto sin error.

### Plugins y panel

| Clave | Tipo | Defecto | Descripción |
|---|---|---|---|
| `plugins` | lista | `clock,greeter` | Plugins a cargar de `plugins/<n>.lua`, separados por comas |
| `panel_modules` | lista | `tasks,clock,date` | Secciones del panel en orden izquierda→derecha |

En ambas listas los espacios tras comas se toleran: `a, b, c` es válido.

**`panel_modules`** admite:
- Nativas: `tasks`, `clock`, `date`
- Slots de plugins: cualquier otro nombre; el plugin escribe ahí con
  `wm.status("nombre", texto)` (ver `docs/PLUGINS.md`)

### Ventanas

| Clave | Tipo | Defecto | Descripción |
|---|---|---|---|
| `window_default_w` | int | `320` | Ancho de ventanas nuevas (Menú → Nuevo) |
| `window_default_h` | int | `200` | Alto de ventanas nuevas |

---

## 3. Archivo de ejemplo completo

```ini
# W3M configuration — key = value, '#' starts a comment

screen_w  = 1024
screen_h  = 768
titlebar_h = 18
taskbar_h  = 24
show_taskbar = true

wallpaper   = teal
theme_fg    = black
theme_bg    = silver
theme_accent = navy

# comma-separated plugin names (plugins/<name>.lua)
plugins = clock,greeter,mem,apps

# panel sections, in left-to-right order. Built-ins: tasks, clock, date.
# Any other name is a slot filled by plugins via wm.status("<name>", text).
panel_modules = tasks,clock,date,mem,apps

window_default_w = 320
window_default_h = 200
```

---

## 4. Recetas de temas

**Win 3.1 clásico:**
```ini
wallpaper = teal
theme_bg = silver
theme_accent = navy
```

**Modo oscuro:**
```ini
wallpaper = black
theme_fg = white
theme_bg = gray
theme_accent = blue
```

**Minimalista monocromo:**
```ini
wallpaper = white
theme_fg = black
theme_bg = white
theme_accent = black
```

**Panel solo con reloj (sin tasks):**
```ini
panel_modules = clock
```

---

## 5. Controles del WM (referencia rápida)

No son configurables aún (MVP); lista fija:

| Acción | Control |
|---|---|
| Nueva ventana | Menú → Nuevo, o **F1** |
| Cerrar ventana | Botón `x` de la barra de título |
| Minimizar / Maximizar | Botones `_` / `^` |
| Mover | Arrastrar la barra de título |
| Redimensionar | Arrastrar borde o esquina |
| Enfoque | Clic en la ventana |
| Alt-Tab | **Alt+Tab** |
| Menú del sistema | Clic en "W3M" (barra de tareas, izquierda) |
| Cerrar menú | **Esc** o clic fuera |
