#!/usr/bin/env bash
# ==============================================================================
#  ProtectEye - Universal Linux Uninstaller
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
    echo -e "${BOLD}Evrensel Linux Kaldırma Sihirbazı${NC}\n"
else
    echo -e "${BOLD}Universal Linux Uninstaller${NC}\n"
fi

# Parse arguments
AUTO_YES=false
for arg in "$@"; do
    if [ "$arg" = "-y" ] || [ "$arg" = "--yes" ]; then
        AUTO_YES=true
    fi
done

# Determine sudo requirement (optional, only used for system-wide paths)
SUDO=""
if [ "$(id -u)" -ne 0 ] && command -v sudo >/dev/null 2>&1; then
    SUDO="sudo"
fi

if [ "$IS_TR" = true ]; then
    echo -e "${YELLOW}==> Çalışan ProtectEye arka plan süreçleri kapatılıyor...${NC}"
else
    echo -e "${YELLOW}==> Stopping background ProtectEye processes...${NC}"
fi

# Stop any running ProtectEye processes, EXCLUDING this script and its caller
CURRENT_PID=$$
PARENT_PID=$PPID
GRANDPARENT_PID=$(ps -o ppid= -p "$PARENT_PID" 2>/dev/null | tr -d ' ' || true)

for pid in $(pgrep -x "$BIN_NAME" 2>/dev/null || true); do
    if [ "$pid" -ne "$CURRENT_PID" ] && [ "$pid" -ne "$PARENT_PID" ] && [ "$pid" -ne "$GRANDPARENT_PID" ]; then
        kill "$pid" 2>/dev/null || true
    fi
done

if [ "$IS_TR" = true ]; then
    echo -e "${BLUE}==> Sistem ve kullanıcı dosyaları kaldırılıyor...${NC}"
else
    echo -e "${BLUE}==> Removing installed binary files and desktop entries...${NC}"
fi

# 1. Remove user-local installation (never requires root or sudo)
rm -f "$HOME/.local/bin/$BIN_NAME"
rm -f "$HOME/.local/share/applications/$BIN_NAME.desktop"
rm -f "$HOME/.local/share/icons/hicolor/scalable/apps/$BIN_NAME.svg"
rm -f "$HOME/.local/share/metainfo/io.github.alierenaltindag.protecteye.metainfo.xml"
rm -f "$HOME/.config/autostart/$BIN_NAME.desktop"

# 2. Remove system-wide files if present and permissions allow
if [ -n "$SUDO" ] || [ "$(id -u)" -eq 0 ]; then
    $SUDO rm -f "/usr/local/bin/$BIN_NAME" "/usr/bin/$BIN_NAME"
    $SUDO rm -f "/usr/local/share/applications/$BIN_NAME.desktop" "/usr/share/applications/$BIN_NAME.desktop"
    $SUDO rm -f "/etc/xdg/autostart/$BIN_NAME.desktop"
    $SUDO rm -f "/usr/local/share/icons/hicolor/scalable/apps/$BIN_NAME.svg" "/usr/share/icons/hicolor/scalable/apps/$BIN_NAME.svg"
    $SUDO rm -f "/usr/local/share/metainfo/io.github.alierenaltindag.protecteye.metainfo.xml" "/usr/share/metainfo/io.github.alierenaltindag.protecteye.metainfo.xml"
    $SUDO rm -f "/usr/local/share/protecteye/uninstall.sh" "/usr/share/protecteye/uninstall.sh"
    $SUDO rm -rf "/usr/local/share/protecteye" "/usr/share/protecteye"
elif [ -f "/usr/local/bin/$BIN_NAME" ] || [ -f "/usr/bin/$BIN_NAME" ]; then
    if [ "$IS_TR" = true ]; then
        echo -e "${YELLOW}[UYARI] Sistem geneli dosyaları (/usr/local/bin) silmek için sudo gereklidir.${NC}"
    else
        echo -e "${YELLOW}[WARNING] Removing system-wide files (/usr/local/bin) requires sudo privileges.${NC}"
    fi
fi

if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    $SUDO gtk-update-icon-cache -q -t -f /usr/local/share/icons/hicolor 2>/dev/null || true
    $SUDO gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor 2>/dev/null || true
fi

if command -v update-desktop-database >/dev/null 2>&1; then
    $SUDO update-desktop-database -q /usr/local/share/applications 2>/dev/null || true
    $SUDO update-desktop-database -q /usr/share/applications 2>/dev/null || true
fi

USER_CONFIG_DIR="$HOME/.config/ProtectEye"
if [ "$(id -u)" -eq 0 ] && [ -n "$SUDO_USER" ] && [ "$SUDO_USER" != "root" ]; then
    USER_CONFIG_DIR="$(getent passwd "$SUDO_USER" | cut -d: -f6)/.config/ProtectEye"
fi

if [ -d "$USER_CONFIG_DIR" ]; then
    CLEAN_CONFIG=false
    if [ "$AUTO_YES" = true ]; then
        CLEAN_CONFIG=true
    elif [ -t 0 ] || [ -e /dev/tty ]; then
        if [ "$IS_TR" = true ]; then
            read -p "Kullanıcı ayarlarını ve verilerini de silmek istiyor musunuz? (e/H): " -r CONFIRM_CLEAN < /dev/tty || CONFIRM_CLEAN="n"
            if [[ "$CONFIRM_CLEAN" =~ ^[EeYy]$ ]]; then
                CLEAN_CONFIG=true
            fi
        else
            read -p "Do you want to remove user configuration and settings as well? (y/N): " -r CONFIRM_CLEAN < /dev/tty || CONFIRM_CLEAN="n"
            if [[ "$CONFIRM_CLEAN" =~ ^[Yy]$ ]]; then
                CLEAN_CONFIG=true
            fi
        fi
    fi

    if [ "$CLEAN_CONFIG" = true ]; then
        rm -rf "$USER_CONFIG_DIR"
        if [ "$IS_TR" = true ]; then
            echo -e "${GREEN}✓ Kullanıcı ayarları temizlendi.${NC}"
        else
            echo -e "${GREEN}✓ User configuration removed.${NC}"
        fi
    fi
fi

if [ "$IS_TR" = true ]; then
    echo -e "\n${GREEN}${BOLD}[TAMAMLANDI] ${APP_NAME} sisteminizden tamamen kaldırıldı.${NC}\n"
else
    echo -e "\n${GREEN}${BOLD}[SUCCESS] ${APP_NAME} has been completely uninstalled from your system.${NC}\n"
fi
