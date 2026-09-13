#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "==> CachyOS / Arch Linux paketi (PKGBUILD) inşa ediliyor..."
makepkg -f --noconfirm

echo "==> Paket başarıyla oluşturuldu:"
ls -lh *.pkg.tar.zst
