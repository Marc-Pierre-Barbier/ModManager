# Maintainer: Marc barbier
pkgname=modmanager
pkgver=0.2
pkgrel=1
pkgdesc="a Mod manager for bethesda games"
arch=('x86_64') # might work on other archs but i can not try
url='https://github.com/Marc-Pierre-Barbier/ModManager'
license=('GPL2')
source=( 'git+https://github.com/Marc-Pierre-Barbier/ModManager.git' )
md5sums=( 'SKIP' )

optdepends=('fuse-overlayfs: special filesystem support' )

depends=( 'glib2' 'unrar' 'p7zip' 'unzip' )

build() {
    cmake -B build -S "ModManager" \
        -DCMAKE_BUILD_TYPE='None' \
        -DCMAKE_INSTALL_PREFIX='/usr' \
        -Wno-dev
    cmake --build build
}

package() {
    DESTDIR="$pkgdir" cmake --install build
}
