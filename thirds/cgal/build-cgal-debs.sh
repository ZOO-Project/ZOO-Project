#!/bin/bash
set -e

# Parámetros
VERSION="5.6"
WORKDIR="$HOME/build-cgal"
PREFIX="/usr"
PKGNAME_SHARED="libcgal-shared"
PKGNAME_DEV="libcgal-dev"

# Instalar dependencias necesarias
sudo apt update
sudo apt install -y git cmake g++ libboost-all-dev libgmp-dev libmpfr-dev \
    fakeroot devscripts dh-make checkinstall

# Limpiar y preparar directorio
rm -rf "$WORKDIR"
mkdir -p "$WORKDIR"
cd "$WORKDIR"

# Clonar CGAL
git clone --branch v$VERSION https://github.com/CGAL/cgal.git
cd cgal
mkdir build && cd build

# Compilar CGAL como biblioteca compartida
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PREFIX -DCGAL_HEADER_ONLY=OFF -DBUILD_SHARED_LIBS=ON
make -j$(nproc)
make DESTDIR="$WORKDIR/stage" install

# Separar archivos entre shared y dev
mkdir -p "$WORKDIR/${PKGNAME_SHARED}/$PREFIX/lib"
mkdir -p "$WORKDIR/${PKGNAME_DEV}/$PREFIX/include"

# Mover librerías compartidas
find "$WORKDIR/stage/$PREFIX/lib" -name "libCGAL*.so*" -exec cp -a {} "$WORKDIR/${PKGNAME_SHARED}/$PREFIX/lib/" \;

# Mover headers
cp -a "$WORKDIR/stage/$PREFIX/include/CGAL" "$WORKDIR/${PKGNAME_DEV}/$PREFIX/include/"

# Symlink para compiladores
if [ -f "$WORKDIR/stage/$PREFIX/lib/libCGAL.so" ]; then
  cp -a "$WORKDIR/stage/$PREFIX/lib/libCGAL.so" "$WORKDIR/${PKGNAME_DEV}/$PREFIX/lib/"
  mkdir -p "$WORKDIR/${PKGNAME_DEV}/DEBIAN"
fi

# Crear control para shared
mkdir -p "$WORKDIR/${PKGNAME_SHARED}/DEBIAN"
cat > "$WORKDIR/${PKGNAME_SHARED}/DEBIAN/control" <<EOF
Package: $PKGNAME_SHARED
Version: $VERSION-1
Section: libs
Priority: optional
Architecture: amd64
Depends: libboost-system1.83.0, libgmp10, libmpfr6
Maintainer: Auto Generated <cgal@local>
Description: CGAL shared library (libCGAL.so)
EOF

# Crear control para dev
mkdir -p "$WORKDIR/${PKGNAME_DEV}/DEBIAN"
cat > "$WORKDIR/${PKGNAME_DEV}/DEBIAN/control" <<EOF
Package: $PKGNAME_DEV
Version: $VERSION-1
Section: libdevel
Priority: optional
Architecture: amd64
Depends: $PKGNAME_SHARED (= $VERSION-1)
Maintainer: Auto Generated <cgal@local>
Description: Development files for CGAL (headers and symlinks)
EOF

# Construir paquetes .deb
dpkg-deb --build "$WORKDIR/$PKGNAME_SHARED"
dpkg-deb --build "$WORKDIR/$PKGNAME_DEV"

echo
echo "✅ Paquetes generados:"
echo "  → $WORKDIR/${PKGNAME_SHARED}.deb"
echo "  → $WORKDIR/${PKGNAME_DEV}.deb"
