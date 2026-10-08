# W3M

**W3M** es un gestor de ventanas de X11 real, minimalista, escrito en C puro
con Xlib — sin dependencias más allá de X11 y Lua. Estética y comportamiento
inspirados en **Windows 3.x**, filosofía de **Openbox/Blackbox**, y plugins
scriptables en Lua con addons de sistema (audio, red, bluetooth, discos).

> W3M es la evolución "real" de
> [win3wm](https://github.com/0ldskoolerz/win3wm) (simulador raylib): el mismo
> core lógico, ahora hablando X11 de verdad: reparenting, decorations propias,
> EWMH básico, taskbar propia.

## Estado

**En desarrollo activo.** Componentes:

| Componente | Estado |
|---|---|
| Core lógico (ventanas, foco, z-order, hit-testing) | ✅ completo, 35 tests |
| Config rc-file + temas (colores X11) | ✅ completo |
| Host de plugins Lua + API `wm.*` + `on_key` | ✅ completo |
| Addons de sistema (audio/red/bluetooth/discos) | ✅ portados |
| Frontend X11: reparenting + frames Win 3.x | ✅ implementado |
| Mover (título) / redimensionar (bordes/esquinas) | ✅ implementado (pointer grab) |
| Taskbar con botones + panel configurable | ✅ implementado |
| Alt+Tab, maximizar/minimizar/cerrar | ✅ implementado |
| EWMH: `_NET_SUPPORTING_WM_CHECK`, `_NET_WM_NAME/PID`, `_NET_SUPPORTED`, `_NET_ACTIVE_WINDOW`, `_NET_CLIENT_LIST`, `_NET_CLOSE_WINDOW`, `_NET_WM_STATE` | ✅ implementado |
| Menú de sistema (taskbar → W3M): Nueva ventana / Cascada / Minimizar todo | ✅ implementado |
| Multi-monitor, multi-desktop (`_NET_WM_DESKTOP`) | 🚧 pendiente |

## Características

- Decoración propia estilo Win 3.x: barra de título, botones `_`/`^`/`x`
- Mover arrastrando la barra de título; redimensionar por bordes/esquinas
- Foco follow-clic + Alt+Tab
- Taskbar propia con botones por ventana y panel configurable
- EWMH básico: taskbars/pagers externos y apps que consultan `_NET` te reconocen
- Menú de sistema Win 3.x: clic en "W3M" (Nueva ventana / Cascada / Minimizar todo)
- Plugins Lua (cada uno en su estado aislado) con eventos y hotkeys
- Addons: volumen (`pactl`), red (`nmcli`), bluetooth (`bluetoothctl`),
  discos (`lsblk`/`udisks2`)
- Configuración rc-file (`config/w3m.conf`)

## Compilar

```sh
# Arch:
sudo pacman -S base-devel libx11 lua pkgconf
make
./w3m config/w3m.conf
```

⚠️ Un WM de X11 solo puede probarse dentro de una sesión X. Para
desarrollo sin salir de tu escritorio, usa Xephyr:

```sh
sudo pacman -S xorg-server-xephyr
Xephyr :1 -screen 1024x768 &
DISPLAY=:1 ./w3m config/w3m.conf
# y dentro: DISPLAY=:1 xterm &
```

## Estructura

```
src/
  wm.c/h          core lógico del WM (sin dependencias, testeado)
  config.c/h      parser rc-file
  plugins.c/h     host Lua + API wm.* + eventos
  x11.h           estado específico de X11
  main.c          frontend Xlib (reparenting, event loop)
plugins/          clock, mem, audio, red, bluetooth, discos, ...
config/w3m.conf   configuración por defecto
tests/            35 checks del core
docs/             guías (portadas de win3wm, ajustadas a X11)
```

## Documentación

- [docs/INSTALL.md](docs/INSTALL.md) — compilación, Xephyr, instalación
- [docs/PLUGINS.md](docs/PLUGINS.md) — API Lua, eventos, addons
- [docs/CONFIG.md](docs/CONFIG.md) — referencia de configuración

## Aplicaciones del escritorio

Los paquetes hermanos completan el escritorio:

- [w3m-apps](https://github.com/0ldskoolerz/w3m-apps) — apps clásicas:
  explorador de archivos (crear/cortar/copiar/comprimir/descomprimir/abrir),
  terminal, administrador de tareas, calculadora y bloc de notas (Xlib
  puro sobre applets busybox)
- [w3m-net](https://github.com/0ldskoolerz/w3m-net) — suite de red
  estilo Trinux: ping, DNS, rutas, ARP, puertos, escaneo LAN, sniffer,
  tráfico, netcat y estado de enlace (11 apps)
- [w3m-linux](https://github.com/0ldskoolerz/w3m-linux) — la distro
  completa: Buildroot + BusyBox + W3M + todas las apps en una ISO de
  ~60 MB

## Relación con win3wm

W3M reutiliza el core lógico de win3wm (wm.c, config.c, plugins.c) verificado
con 35 tests unitarios. El frontend raylib queda como simulador/demos de win3wm;
W3M es el WM de producción para X11.
