# Maintainer: 0ldskoolerz <0ldskoolerz@users.noreply.github.com>
pkgname=w3m-wm
pkgver=0.5.0
pkgrel=1
pkgdesc="W3M — gestor de ventanas X11 minimalista estilo Windows 3.x (Xlib puro + plugins Lua)"
arch=('x86_64' 'i686' 'aarch64')
url="https://github.com/0ldskoolerz/W3M"
license=('MIT')
depends=('libx11' 'lua')
makedepends=('git' 'pkgconf')
source=("$pkgname::git+$url.git#tag=v$pkgver")
md5sums=('SKIP')

build() {
    cd "$pkgname"
    make
}

package() {
    cd "$pkgname"
    install -Dm755 w3m "$pkgdir/usr/bin/w3m-wm"
    install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
    for d in INSTALL PLUGINS CONFIG; do
        install -Dm644 docs/$d.md "$pkgdir/usr/share/doc/$pkgname/$d.md"
    done
    install -Dm644 config/w3m.conf "$pkgdir/usr/share/$pkgname/w3m.conf"
    install -dm755 "$pkgdir/usr/share/$pkgname/plugins"
    for f in plugins/*.lua plugins/README.md; do
        install -Dm644 "$f" "$pkgdir/usr/share/$pkgname/$f"
    done
}
