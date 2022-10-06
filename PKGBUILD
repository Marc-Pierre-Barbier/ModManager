# Maintainer: Marc barbier
pkgname=modmanager
pkgver=0.1
pkgrel=1
pkgdesc="a Mod manager for bethesda games"
arch=('x86_64') # might work on other archs but i can not try
url='https://gitlab.marcbarbier.fr/Marc/modmanager'
license=('GPL2')
source=( 'git+https://gitlab.marcbarbier.fr/Marc/modmanager.git' )
md5sums=( 'SKIP' )

optdepends=('fuse-overlayfs: special filesystem support' )

depends=( 'glib2' 'unrar' 'p7zip' 'unzip' )

build() {
    cmake -B build -S "${pkgname}" \
        -DCMAKE_BUILD_TYPE='None' \
        -DCMAKE_INSTALL_PREFIX='/usr' \
        -Wno-dev
    cmake --build build
}

package() {
    DESTDIR="$pkgdir" cmake --install build
}
