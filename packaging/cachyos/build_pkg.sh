#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$SCRIPT_DIR"

if [ -f "$ROOT_DIR/VERSION" ]; then
    PKGVER=$(cat "$ROOT_DIR/VERSION" | tr -d '[:space:]')
    sed -i "s/^pkgver=.*/pkgver=${PKGVER}/" PKGBUILD
fi

echo "==> CachyOS / Arch Linux paketi (PKGBUILD) inşa ediliyor..."
makepkg -f --noconfirm

echo "==> Paket başarıyla oluşturuldu:"
ls -lh *.pkg.tar.zst
