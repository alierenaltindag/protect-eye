#!/bin/bash
# ==============================================================================
#  ProtectEye - Single Source of Truth Version Management Script
#  Updates VERSION and synchronizes all package manifests automatically.
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
VERSION_FILE="$ROOT_DIR/VERSION"

if [ ! -f "$VERSION_FILE" ]; then
    echo "ERROR: $VERSION_FILE not found!" >&2
    exit 1
fi

CURRENT_VER=$(cat "$VERSION_FILE" | tr -d '[:space:]')

if [ -z "$1" ]; then
    echo "ProtectEye Current Version: $CURRENT_VER"
    echo ""
    echo "Usage:"
    echo "  $0 <new_version>       (e.g., $0 1.0.5)"
    echo "  $0 patch               (increments patch: 1.0.4 -> 1.0.5)"
    echo "  $0 minor               (increments minor: 1.0.4 -> 1.1.0)"
    echo "  $0 major               (increments major: 1.0.4 -> 2.0.0)"
    exit 0
fi

# Parse SemVer
IFS='.' read -r MAJOR MINOR PATCH <<< "$CURRENT_VER"
MAJOR=${MAJOR:-0}
MINOR=${MINOR:-0}
PATCH=${PATCH:-0}

TARGET="$1"
case "$TARGET" in
    patch)
        NEW_VER="$MAJOR.$MINOR.$((PATCH + 1))"
        ;;
    minor)
        NEW_VER="$MAJOR.$((MINOR + 1)).0"
        ;;
    major)
        NEW_VER="$((MAJOR + 1)).0.0"
        ;;
    v*|V*)
        NEW_VER="${TARGET#v}"
        NEW_VER="${NEW_VER#V}"
        ;;
    *)
        NEW_VER="$TARGET"
        ;;
esac

# Validate SemVer format
if ! [[ "$NEW_VER" =~ ^[0-9]+\.[0-9]+\.[0-9]+([a-zA-Z0-9\.\-]*)?$ ]]; then
    echo "ERROR: Invalid version format: '$NEW_VER'. Must be X.Y.Z (SemVer)" >&2
    exit 1
fi

echo "========================================================"
echo "  Bumping ProtectEye Version: $CURRENT_VER -> $NEW_VER"
echo "========================================================"

# 1. Update root VERSION (Single Source of Truth)
echo "$NEW_VER" > "$VERSION_FILE"
echo "  [✓] Updated $VERSION_FILE"

# 2. Update Arch Linux PKGBUILD
PKGBUILD_FILE="$ROOT_DIR/packaging/cachyos/PKGBUILD"
if [ -f "$PKGBUILD_FILE" ]; then
    sed -i "s/^pkgver=.*/pkgver=${NEW_VER}/" "$PKGBUILD_FILE"
    echo "  [✓] Updated $PKGBUILD_FILE"
fi

# 3. Update AppStream MetaInfo XML (<release version="X.Y.Z" date="...">)
METAINFO_FILE="$ROOT_DIR/io.github.alierenaltindag.protecteye.metainfo.xml"
if [ -f "$METAINFO_FILE" ]; then
    TODAY=$(date +%Y-%m-%d)
    if ! grep -q "version=\"$NEW_VER\"" "$METAINFO_FILE"; then
        sed -i "/<releases>/a \    <release version=\"$NEW_VER\" date=\"$TODAY\">\n      <description>\n        <p>ProtectEye release v$NEW_VER.</p>\n      </description>\n    </release>" "$METAINFO_FILE"
        echo "  [✓] Updated $METAINFO_FILE"
    fi
fi

# 4. Trigger CMake configuration to update project version
if command -v cmake >/dev/null 2>&1 && [ -d "$ROOT_DIR/build" ]; then
    echo "==> Re-configuring CMake build with new version..."
    cmake -B "$ROOT_DIR/build" -G Ninja >/dev/null
    echo "  [✓] CMake re-configured"
fi

echo ""
echo "Version successfully updated to $NEW_VER across all manifests!"
echo ""
echo "Next steps to release:"
echo "  git add VERSION packaging/cachyos/PKGBUILD io.github.alierenaltindag.protecteye.metainfo.xml"
echo "  git commit -m \"chore(release): bump version to v$NEW_VER\""
echo "  git tag -a \"v$NEW_VER\" -m \"Release v$NEW_VER\""
echo "  git push origin main --tags"
