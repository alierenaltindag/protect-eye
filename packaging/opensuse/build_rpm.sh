#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "==> openSUSE (.rpm) paketi oluşturuluyor..."
cmake -B "$ROOT_DIR/build" -S "$ROOT_DIR" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr

cmake --build "$ROOT_DIR/build"

cd "$ROOT_DIR/build"
cpack -G RPM

RPM_FILE=$(ls protecteye-*.rpm 2>/dev/null | head -1)

if [ -n "$RPM_FILE" ]; then
    echo "==> openSUSE .rpm paketi başarıyla üretildi: $RPM_FILE"
    cp -f "$RPM_FILE" "$SCRIPT_DIR/"
    ls -lh "$SCRIPT_DIR/"*.rpm
else
    echo "==> Hata: .rpm paketi üretilemedi. 'rpm-build' paketinin kurulu olduğundan emin olun."
    exit 1
fi
