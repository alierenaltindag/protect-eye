#!/usr/bin/env bash
# ==============================================================================
#  ProtectEye - Universal Linux Installer
#  Supports: Ubuntu, Debian, CachyOS, Arch, Fedora, openSUSE & derivatives
# ==============================================================================

set -e

APP_NAME="ProtectEye"
BIN_NAME="protecteye"

# Detect System Language
SYS_LANG="${LANG:-${LC_ALL:-${LC_MESSAGES:-en}}}"
if [[ "${SYS_LANG,,}" =~ ^tr ]]; then
    IS_TR=true
else
    IS_TR=false
fi

# Colors for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
BOLD='\033[1m'
NC='\033[0m' # No Color

echo -e "${CYAN}${BOLD}"
echo "  ____            _            _     _____             "
echo " |  _ \ _ __ ___ | |_ ___  ___| |_  | ____|   _  ___   "
echo " | |_) | '__/ _ \| __/ _ \/ __| __| |  _|| | | |/ _ \  "
echo " |  __/| | | (_) | ||  __/ (__| |_  | |___ |_| |  __/  "
echo " |_|   |_|  \___/ \__\___|\___|\__| |_____|\__, |\___|  "
echo "                                           |___/       "
echo -e "${NC}"

if [ "$IS_TR" = true ]; then
    echo -e "${BOLD}Evrensel Linux Kurulum Sihirbazı${NC}\n"
else
    echo -e "${BOLD}Universal Linux Installer & Setup${NC}\n"
fi

# Parse arguments
USER_MODE=false
for arg in "$@"; do
    if [ "$arg" = "--user" ]; then
        USER_MODE=true
    fi
done

# Determine sudo requirement:
# If user explicitly requested --user, or if sudo is not available, install to user-space (~/.local) without root.
SUDO=""
if [ "$(id -u)" -ne 0 ]; then
    if [ "$USER_MODE" = true ] || ! command -v sudo >/dev/null 2>&1; then
        USER_MODE=true
        SUDO=""
        if [ "$IS_TR" = true ]; then
            echo -e "${CYAN}[BİLGİ] Kullanıcı modunda (~/.local) kuruluyor (Root/sudo yetkisi gerekmez).${NC}\n"
        else
            echo -e "${CYAN}[INFO] Installing in user-local mode (~/.local) - No root/sudo privileges needed.${NC}\n"
        fi
    else
        SUDO="sudo"
    fi
fi

# Handle --uninstall flag
if [ "$1" = "--uninstall" ] || [ "$1" = "-u" ]; then
    if [ "$IS_TR" = true ]; then
        echo -e "${YELLOW}==> ${APP_NAME} kaldırılıyor...${NC}"
    else
        echo -e "${YELLOW}==> Uninstalling ${APP_NAME}...${NC}"
    fi

    CURRENT_PID=$$
    PARENT_PID=$PPID
    GRANDPARENT_PID=$(ps -o ppid= -p "$PARENT_PID" 2>/dev/null | tr -d ' ' || true)

    for pid in $(pgrep -x "$BIN_NAME" 2>/dev/null || true); do
        if [ "$pid" -ne "$CURRENT_PID" ] && [ "$pid" -ne "$PARENT_PID" ] && [ "$pid" -ne "$GRANDPARENT_PID" ]; then
            kill "$pid" 2>/dev/null || true
        fi
    done


    $SUDO rm -f "/usr/local/bin/$BIN_NAME"
    $SUDO rm -f "/usr/bin/$BIN_NAME"
    $SUDO rm -f "/usr/local/share/applications/$BIN_NAME.desktop"
    $SUDO rm -f "/usr/share/applications/$BIN_NAME.desktop"
    $SUDO rm -f "/etc/xdg/autostart/$BIN_NAME.desktop"
    $SUDO rm -f "$HOME/.config/autostart/$BIN_NAME.desktop"
    $SUDO rm -f "/usr/local/share/icons/hicolor/scalable/apps/$BIN_NAME.svg"
    $SUDO rm -f "/usr/share/icons/hicolor/scalable/apps/$BIN_NAME.svg"

    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        $SUDO gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor || true
    fi
    if command -v update-desktop-database >/dev/null 2>&1; then
        $SUDO update-desktop-database -q /usr/share/applications || true
    fi

    if [ "$IS_TR" = true ]; then
        echo -e "${GREEN}[TAMAMLANDI] ${APP_NAME} sisteminizden tamamen kaldırıldı.${NC}"
    else
        echo -e "${GREEN}[OK] ${APP_NAME} has been completely uninstalled.${NC}"
    fi
    exit 0
fi

# Locate repository / source directory
if [ -n "${BASH_SOURCE[0]}" ] && [ -f "${BASH_SOURCE[0]}" ] && [ -f "$(dirname "${BASH_SOURCE[0]}")/CMakeLists.txt" ]; then
    # Running directly from a local cloned repository: ./installer.sh
    SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
else
    # Running piped via curl | bash or without local files -> fetch latest release tag
    if [ "$IS_TR" = true ]; then
        echo -e "${BLUE}==> En güncel ProtectEye sürümü kontrol ediliyor...${NC}"
    else
        echo -e "${BLUE}==> Fetching latest ProtectEye release...${NC}"
    fi

    LATEST_TAG=$(curl -fsSL -H "Cache-Control: no-cache" https://api.github.com/repos/alierenaltindag/protect-eye/releases/latest \
                 | grep '"tag_name"' | head -1 | cut -d'"' -f4)

    if [ -z "$LATEST_TAG" ]; then
        if [ "$IS_TR" = true ]; then
            echo -e "${YELLOW}[UYARI] En son sürüm etiketi alınamadı, ana dal (main) indiriliyor.${NC}"
        else
            echo -e "${YELLOW}[WARNING] Could not determine latest release tag, falling back to main branch.${NC}"
        fi
        LATEST_TAG="main"
    else
        if [ "$IS_TR" = true ]; then
            echo -e "${GREEN}==> ProtectEye ${LATEST_TAG} kuruluyor...${NC}"
        else
            echo -e "${GREEN}==> Installing ProtectEye ${LATEST_TAG}...${NC}"
        fi
    fi

    TEMP_DIR=$(mktemp -d)
    git -c advice.detachedHead=false clone --quiet --depth 1 --branch "$LATEST_TAG" https://github.com/alierenaltindag/protect-eye.git "$TEMP_DIR" 2>/dev/null || \
    git clone --depth 1 --branch "$LATEST_TAG" https://github.com/alierenaltindag/protect-eye.git "$TEMP_DIR"
    SRC_DIR="$TEMP_DIR"
    trap 'rm -rf "$TEMP_DIR"' EXIT

fi

# Detect Linux Distribution
DISTRO="unknown"
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO="${ID,,}"
    DISTRO_LIKE="${ID_LIKE,,}"
fi

if [ "$IS_TR" = true ]; then
    echo -e "${BLUE}==> Algılanan Linux Dağıtımı: ${BOLD}${PRETTY_NAME:-$DISTRO}${NC}"
else
    echo -e "${BLUE}==> Detected Linux Distribution: ${BOLD}${PRETTY_NAME:-$DISTRO}${NC}"
fi

install_dependencies() {
    if [ "$IS_TR" = true ]; then
        echo -e "${YELLOW}==> Gerekli paket bağımlılıkları kontrol ediliyor ve kuruluyor...${NC}"
    else
        echo -e "${YELLOW}==> Checking and installing required dependencies...${NC}"
    fi

    if command -v pacman >/dev/null 2>&1; then
        $SUDO pacman -S --needed --noconfirm cmake ninja gcc qt6-base qt6-multimedia qt6-svg
    elif command -v apt-get >/dev/null 2>&1; then
        $SUDO apt-get update
        $SUDO apt-get install -y cmake ninja-build build-essential qt6-base-dev qt6-multimedia-dev libqt6svg6-dev
    elif command -v dnf >/dev/null 2>&1; then
        $SUDO dnf install -y cmake ninja-build gcc-c++ qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qtsvg-devel
    elif command -v zypper >/dev/null 2>&1; then
        $SUDO zypper in -y cmake ninja gcc-c++ qt6-base-devel qt6-multimedia-devel qt6-svg-devel
    else
        if [ "$IS_TR" = true ]; then
            echo -e "${YELLOW}[UYARI] Bilinmeyen paket yöneticisi. Lütfen CMake, Ninja ve Qt6 kütüphanelerini elle kurun.${NC}"
        else
            echo -e "${YELLOW}[WARNING] Unknown package manager. Please ensure CMake, Ninja, and Qt6 are installed.${NC}"
        fi
    fi
}

# Check if build tools and all Qt6 development modules are ready
if ! command -v cmake >/dev/null 2>&1 || \
    ! command -v pkg-config >/dev/null 2>&1 || \
    ! pkg-config --exists Qt6Core Qt6Gui Qt6Widgets Qt6Multimedia Qt6Svg Qt6Network Qt6DBus; then
    if [ "$USER_MODE" = true ]; then
        if [ "$IS_TR" = true ]; then
            echo -e "${RED}[HATA] Gerekli derleme araçları (CMake veya pkg-config) sistemde bulunamadı.${NC}"
            echo -e "Kullanıcı modunda kurulum için sistemde 'cmake', 'ninja' ve 'qt6' önceden kurulu olmalıdır."
            echo -e "Lütfen sistem yöneticinizden bu paketleri kurmasını isteyin veya 'sudo ./installer.sh' ile çalıştırın."
        else
            echo -e "${RED}[ERROR] Required build tools (CMake or pkg-config) were not found on this system.${NC}"
            echo -e "For user-space installation, 'cmake', 'ninja', and 'qt6' must already be present."
            echo -e "Please ask your administrator to install them or run with 'sudo ./installer.sh'."
        fi
        exit 1
    fi
    install_dependencies
fi

# Build
if [ "$IS_TR" = true ]; then
    echo -e "${BLUE}==> ${APP_NAME} Release modunda derleniyor...${NC}"
else
    echo -e "${BLUE}==> Building ${APP_NAME} in Release mode...${NC}"
fi

GENERATOR="Unix Makefiles"
if command -v ninja >/dev/null 2>&1; then
    GENERATOR="Ninja"
fi

if [ "$USER_MODE" = true ]; then
    INSTALL_PREFIX="$HOME/.local"
    mkdir -p "$INSTALL_PREFIX/bin"
    mkdir -p "$INSTALL_PREFIX/share/applications"
    mkdir -p "$INSTALL_PREFIX/share/icons/hicolor/scalable/apps"
    mkdir -p "$INSTALL_PREFIX/share/metainfo"
    mkdir -p "$HOME/.config/autostart"
else
    INSTALL_PREFIX="/usr/local"
fi

cmake -B "$SRC_DIR/build" -S "$SRC_DIR" \
    -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"

cmake --build "$SRC_DIR/build" --parallel "$(nproc)"

# Install
if [ "$USER_MODE" = true ]; then
    if [ "$IS_TR" = true ]; then
        echo -e "${BLUE}==> ${APP_NAME} kullanıcı dizinine (~/.local) kuruluyor...${NC}"
    else
        echo -e "${BLUE}==> Installing ${APP_NAME} to user directory (~/.local)...${NC}"
    fi

    cmake --install "$SRC_DIR/build"

    # User autostart
    cp -f "$SRC_DIR/resources/protecteye-autostart.desktop" "$HOME/.config/autostart/protecteye.desktop"

    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        gtk-update-icon-cache -q -t -f "$HOME/.local/share/icons/hicolor" 2>/dev/null || true
    fi
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database -q "$HOME/.local/share/applications" 2>/dev/null || true
    fi

    PROTECTEYE_BIN="$HOME/.local/bin/protecteye"
else
    if [ "$IS_TR" = true ]; then
        echo -e "${BLUE}==> ${APP_NAME} sistem dizinlerine (/usr/local) kuruluyor...${NC}"
    else
        echo -e "${BLUE}==> Installing ${APP_NAME} to system directories (/usr/local)...${NC}"
    fi

    $SUDO cmake --install "$SRC_DIR/build"

    # Ensure autostart in /etc/xdg/autostart
    $SUDO mkdir -p /etc/xdg/autostart
    $SUDO cp -f "$SRC_DIR/resources/protecteye-autostart.desktop" /etc/xdg/autostart/protecteye.desktop

    # Update system icon and desktop caches
    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        $SUDO gtk-update-icon-cache -q -t -f /usr/local/share/icons/hicolor 2>/dev/null || true
        $SUDO gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor 2>/dev/null || true
    fi

    if command -v update-desktop-database >/dev/null 2>&1; then
        $SUDO update-desktop-database -q /usr/local/share/applications 2>/dev/null || true
        $SUDO update-desktop-database -q /usr/share/applications 2>/dev/null || true
    fi

    PROTECTEYE_BIN="/usr/local/bin/protecteye"
    if ! command -v protecteye >/dev/null 2>&1; then
        PROTECTEYE_BIN="/usr/bin/protecteye"
    fi
fi

# Launch ProtectEye immediately in the background as the regular user
if [ "$(id -u)" -eq 0 ]; then
    if [ -n "$SUDO_USER" ] && [ "$SUDO_USER" != "root" ]; then
        su - "$SUDO_USER" -c "nohup \"$PROTECTEYE_BIN\" >/dev/null 2>&1 &"
    fi
else
    nohup "$PROTECTEYE_BIN" >/dev/null 2>&1 &
fi

APP_VER=$("$PROTECTEYE_BIN" --version 2>/dev/null | awk '{print $2}')
[ -z "$APP_VER" ] && APP_VER="${LATEST_TAG:-v1.0.0}"

if [ "$IS_TR" = true ]; then
    echo -e "\n${GREEN}${BOLD}✓ ProtectEye ${APP_VER} başarıyla kuruldu ve sistem tepsisinde çalışıyor!${NC}\n"
    echo -e "  ${CYAN}• Yardım ve Seçenekler:${NC}  ${BOLD}protecteye --help${NC}"
    echo -e "  ${CYAN}• Sistem Tepsisi:${NC}        Ayarları açmak ve molaları özelleştirmek için simgeye tıklayın"
    echo -e "  ${CYAN}• Programı Kaldır:${NC}       ${BOLD}protecteye uninstall${NC}\n"
else
    echo -e "\n${GREEN}${BOLD}✓ ProtectEye ${APP_VER} is now installed and running in your system tray!${NC}\n"
    echo -e "  ${CYAN}• Help & Options:${NC}  ${BOLD}protecteye --help${NC}"
    echo -e "  ${CYAN}• System Tray:${NC}     Click or right-click the tray icon to customize"
    echo -e "  ${CYAN}• Uninstall:${NC}       ${BOLD}protecteye uninstall${NC}\n"
fi
