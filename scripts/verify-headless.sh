#!/bin/sh
# verify-headless.sh — instala y prueba W3M en un servidor sin graficos.
# Requiere: xorg-server-xvfb xterm xdotool wmctrl
# Uso: ./scripts/verify-headless.sh
set -e
cd "$(dirname "$0")/.."

echo "== compilando W3M (no requiere display)"
make >/dev/null

echo "== arrancando Xvfb en :99"
Xvfb :99 -screen 0 1024x768x24 >/dev/null 2>&1 &
XVFB_PID=$!
sleep 1
export DISPLAY=:99

echo "== lanzando W3M"
./w3m config/w3m.conf >/tmp/w3m.log 2>&1 &
W3M_PID=$!
sleep 1

echo "== lanzando clientes (xterm)"
xterm &
sleep 1
xterm &
sleep 1

echo "== verificacion EWMH (debe decir: Name: W3M)"
wmctrl -m || { echo "FALLO: wmctrl no ve el WM"; kill $W3M_PID $XVFB_PID; exit 1; }

echo "== lista de ventanas manejadas"
wmctrl -l || true

echo "== Alt+Tab sintetico"
xdotool key alt+Tab
sleep 0.3

echo "== click en los botones de titulo de la ventana activa (cerrar)"
ID=$(wmctrl -l | awk 'NR==1{print $1}')
wmctrl -i -a "$ID"
sleep 0.2
# coordenadas relativas al frame: esquina sup-derecha ~ boton cerrar
GEO=$(xdotool getwindowgeometry "$ID" | grep Position | awk '{print $2}' | tr ',' ' ')
X=$(echo $GEO | cut -d' ' -f1); Y=$(echo $GEO | cut -d' ' -f2)
xdotool mousemove $((X + 500)) $((Y + 12)) click 1
sleep 0.5

echo "== captura de pantalla"
xwd -root -out /tmp/w3m-shot.xwd && echo "captura: /tmp/w3m-shot.xwd (convertir con: convert /tmp/w3m-shot.xwd shot.png)"

echo "== el proceso sigue vivo?"
if kill -0 $W3M_PID 2>/dev/null; then
    echo "OK: W3M corriendo sin crash"
    kill $W3M_PID
else
    echo "FALLO: W3M murio — log:"
    cat /tmp/w3m.log
fi
kill $XVFB_PID 2>/dev/null || true
echo "== verificacion terminada (log completo: /tmp/w3m.log)"
