#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "==> Ubuntu/Debian (.deb) paketi oluşturuluyor..."
cmake -B "$ROOT_DIR/build" -S "$ROOT_DIR" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCPACK_DEBIAN_PACKAGE_ARCHITECTURE=amd64

cmake --build "$ROOT_DIR/build"

cd "$ROOT_DIR/build"
cpack -G DEB

echo "==> Ubuntu/Debian/Pardus (.deb) paketi başarıyla üretildi:"
VERSION=$(grep "PROJECT_VERSION" "$ROOT_DIR/CMakeLists.txt" | head -1 | sed -E 's/.*VERSION[[:space:]]+([0-9.]+).*/\1/')
DEB_FILE=$(ls protecteye-*.deb 2>/dev/null | head -1)
if [ -n "$DEB_FILE" ]; then
    cp -f "$DEB_FILE" "$SCRIPT_DIR/protecteye_${VERSION}_amd64.deb"
    ls -lh "$SCRIPT_DIR/protecteye_${VERSION}_amd64.deb"
fi
