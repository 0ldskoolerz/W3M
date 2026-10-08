# W3M — Instalar y verificar en un Linux sin entorno gráfico (CachyOS/Arch)

Esta guía cubre el ciclo completo en un servidor o VM **sin monitor ni
escritorio**: instalar dependencias, instalar W3M (como paquete o compilado),
ejecutarlo sobre un servidor X virtual (Xvfb) y validar que funciona
(EWMH, decoración, interacción simulada) — todo headless.

---

## 1. Requisitos

### Compilar W3M

```sh
sudo pacman -S base-devel libx11 lua pkgconf
```

### Ejecutar headless (X virtual + clientes de prueba + herramientas)

```sh
sudo pacman -S xorg-server-xvfb xterm xdotool wmctrl
```

| Paquete | Para qué |
|---|---|
| `base-devel libx11 lua` | compilar W3M |
| `xorg-server-xvfb` | servidor X virtual (tu "pantalla" sin monitor) |
| `xterm` | cliente de prueba que W3M decorará |
| `xdotool` | simular teclado y ratón |
| `wmctrl` | consultar EWMH y listar ventanas |

> ⚠️ W3M **compila sin display**, pero **no se ejecuta sin un servidor X
> activo**. Xvfb es ese servidor. No lances `./w3m` sin `DISPLAY` válido.

---

## 2. Instalar W3M

### Opción A — paquete pacman (recomendada)

```sh
git clone https://github.com/0ldskoolerz/W3M.git
cd W3M
makepkg -si
```

Instala `/usr/bin/w3m-wm` y config/plugins de ejemplo en `/usr/share/w3m-wm/`.

### Opción B — compilar en sitio (para iterar rápido)

```sh
git clone https://github.com/0ldskoolerz/W3M.git
cd W3M
make            # produce ./w3m
```

---

## 3. Verificación automática (una línea)

El repo incluye un script que hace todo el ciclo:

```sh
./scripts/verify-headless.sh
```

Ejecuta: compilar → Xvfb en `:99` → lanzar W3M → abrir 2 xterms (que W3M
decora con frames Win 3.x) → validar EWMH con `wmctrl -m` (debe imprimir
`Name: W3M`) → listar ventanas → simular Alt+Tab y un click en el botón
cerrar con `xdotool` → capturar pantalla → confirmar que no crasheó.

Log completo del WM en `/tmp/w3m.log`; captura en `/tmp/w3m-shot.xwd`.

---

## 4. Ejecución manual, paso a paso

```sh
Xvfb :99 -screen 0 1024x768x24 &      # pantalla virtual
export DISPLAY=:99
./w3m config/w3m.conf &               # W3M toma el display
xterm &                                # W3M lo adopta y decora
xterm &

wmctrl -m                             # → Name: W3M  (EWMH OK)
wmctrl -l                             # → lista las ventanas manejadas
```

### Ver qué está pasando (sin monitor)

```sh
xwd -root -out /tmp/shot.xwd
sudo pacman -S imagemagick             # si no lo tienes
convert /tmp/shot.xwd /tmp/shot.png    # copia shot.png a tu máquina y ábrela
```

### Verlo en vivo desde tu PC con escritorio (VNC por túnel SSH)

```sh
# en el servidor CachyOS: VNC solo-local sobre el Xvfb
sudo pacman -S x11vnc
x11vnc -display :99 -localhost -nopw &

# en tu PC:
ssh -L 5900:localhost:5900 usuario@cachyos
# conecta tu cliente VNC a localhost:5900
```

⚠️ Nunca expongas el VNC fuera de localhost sin contraseña. El túnel SSH
es la parte que lo mantiene seguro.

---

## 5. Qué esperar y cómo interpretar fallos

| Comprobación | Éxito | Si falla |
|---|---|---|
| `wmctrl -m` | `Name: W3M` | `N/A` → W3M no registró EWMH; revisa `/tmp/w3m.log` |
| `wmctrl -l` | Lista las xterms con sus títulos | Vacío → W3M no adoptó clientes |
| Proceso vivo tras 10 s | `kill -0` OK | Murió → guarda `/tmp/w3m.log` y repórtalo |
| Captura xwd | Se ve taskbar + frames Win 3.x | Frames vacíos → problema de render |

Ante cualquier fallo en runtime, abre un issue en el repo adjuntando:
salida del script, contenido de `/tmp/w3m.log` y versión de Xvfb
(`Xvfb -version`).

---

## 6. Relación con Xephyr

Xvfb es para **servidores sin monitor** (X en memoria, nadie lo ve).
**Xephyr** es para **probar dentro de tu escritorio** (ventana que contiene
un X). Ambos sirven para validar W3M; usa Xephyr si tienes gráficos, Xvfb
si no.
