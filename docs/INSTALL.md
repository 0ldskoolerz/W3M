# W3M — Guía de compilación e instalación (X11)

## 1. Requisitos

| Paquete | Propósito |
|---|---|
| `base-devel` | gcc, make, pkgconf |
| `libx11` | Xlib (el WM habla X11 directo) |
| `lua` | plugins (5.3/5.4; opcional pero recomendado) |

Opcionales para los addons:
`networkmanager`, `bluez-utils`, `pipewire-pulseaudio`, `udisks2`.

NO necesitas: GTK/Qt, xcb-utils, drivers extra, root.

## 2. Compilar

```sh
sudo pacman -S base-devel libx11 lua pkgconf
make
```

## 3. Probar W3M de forma segura

⚠️ **Un WM toma control de todo el display**: no lo ejecutes en tu sesión actual salvo que
sepas lo que haces (perderías el control de tus ventanas al salir mal).
Usa **Xephyr** (servidor X anidado) para desarrollo:

```sh
sudo pacman -S xorg-server-xephyr
Xephyr :1 -screen 1024x768 &
DISPLAY=:1 ./w3m config/w3m.conf &
# lanza apps dentro del WM anidado:
DISPLAY=:1 xterm &
```

### Ejecutarlo como tu WM real

En `~/.xinitrc`:

```sh
exec /ruta/a/w3m /ruta/a/config/w3m.conf
```

y arranca con `startx`. También compatible con gestores de login
(entrada "custom session" apuntando a ese script).

## 4. Instalar como paquete pacman

```sh
git clone https://github.com/0ldskoolerz/W3M.git
cd W3M
makepkg -si
```

Instala: `/usr/bin/w3m-wm`, docs en `/usr/share/doc/w3m-wm/`, config y
plugins de ejemplo en `/usr/share/w3m-wm/`.

## 5. Solución de problemas

| Síntoma | Causa | Solución |
|---|---|---|
| `no se pudo abrir el display X11` | sin DISPLAY o ya hay un WM | usa Xephyr con `DISPLAY=:1` |
| Ventanas sin decorar | otro WM activo | W3M debe ser el único WM del display |
| No reacciona al ratón en bordes | aún no implementado el resize X11 (en desarrollo) | ver README estado |
| Plugins no cargan | falta lua o CWD sin plugins/ | instala lua, corre desde la raíz del proyecto |
