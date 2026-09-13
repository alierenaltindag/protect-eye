#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "==> Pardus / Debian (.deb) paketi oluşturuluyor..."
cmake -B "$ROOT_DIR/build" -S "$ROOT_DIR" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCPACK_DEBIAN_PACKAGE_ARCHITECTURE=amd64

cmake --build "$ROOT_DIR/build"

cd "$ROOT_DIR/build"
cpack -G DEB

DEB_FILE=$(ls protecteye-*.deb 2>/dev/null | head -1)

if [ -n "$DEB_FILE" ]; then
    echo "==> Pardus .deb paketi başarıyla üretildi: $DEB_FILE"
    cp -f "$DEB_FILE" "$SCRIPT_DIR/"
    ls -lh "$SCRIPT_DIR/"*.deb
else
    echo "==> Hata: .deb paketi üretilemedi."
    exit 1
fi
