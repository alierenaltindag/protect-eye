#!/bin/bash
# ==============================================================================
#  ProtectEye - Build All Distribution Release Packages
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
RELEASE_DIR="$ROOT_DIR/releases"

mkdir -p "$RELEASE_DIR"

echo "========================================================"
echo "      ProtectEye - Building GitHub Release Packages     "
echo "========================================================"

# 1. Build CachyOS / Arch Linux package (.pkg.tar.zst)
if [ -f "$SCRIPT_DIR/cachyos/build_pkg.sh" ]; then
    echo -e "\n==> [1/2] Building CachyOS / Arch Linux package..."
    "$SCRIPT_DIR/cachyos/build_pkg.sh"
    cp -f "$SCRIPT_DIR/cachyos/"*.pkg.tar.zst "$RELEASE_DIR/"
fi

# 2. Build Ubuntu / Debian / Pardus package (.deb)
if [ -f "$SCRIPT_DIR/ubuntu/build_deb.sh" ]; then
    echo -e "\n==> [2/3] Building Ubuntu / Debian / Pardus (.deb) package..."
    "$SCRIPT_DIR/ubuntu/build_deb.sh"
    cp -f "$SCRIPT_DIR/ubuntu/"*.deb "$RELEASE_DIR/"
fi

# 3. Build Fedora / openSUSE / Red Hat package (.rpm)
if [ -f "$SCRIPT_DIR/fedora/build_rpm.sh" ]; then
    if command -v rpmbuild >/dev/null 2>&1; then
        echo -e "\n==> [3/3] Building Fedora / openSUSE / RHEL (.rpm) package..."
        "$SCRIPT_DIR/fedora/build_rpm.sh"
        cp -f "$SCRIPT_DIR/fedora/"*.rpm "$RELEASE_DIR/"
    else
        echo -e "\n[INFO] 'rpmbuild' bulunamadı, Fedora (.rpm) yerel derlemesi atlanıyor."
    fi
fi

echo -e "\n========================================================"
echo "                 RELEASE ARTIFACTS                      "
echo "========================================================"
ls -lh "$RELEASE_DIR"

echo -e "\n==> SHA256 Checksums:"
cd "$RELEASE_DIR"
sha256sum * 2>/dev/null || true

echo -e "\n[INFO] For Windows: run 'packaging\\windows\\build_windows.bat' on a Windows machine"
echo "       to produce 'ProtectEye_Setup.exe' and place it in the 'releases/' folder."
