#!/usr/bin/env bash
# ==============================================================================
#  ProtectEye - Universal Linux Updater
#  Checks GitHub Releases and automatically updates ProtectEye.
# ==============================================================================

set -e

APP_NAME="ProtectEye"
BIN_NAME="protecteye"
REPO="alierenaltindag/protect-eye"

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
    echo -e "${BOLD}ProtectEye Otomatik Güncelleme Aracı${NC}\n"
else
    echo -e "${BOLD}ProtectEye Auto-Update Tool${NC}\n"
fi

# Parse arguments
AUTO_YES=false
CHECK_ONLY=false
for arg in "$@"; do
    case "$arg" in
        -y|--yes)
            AUTO_YES=true
            ;;
        --check|-c)
            CHECK_ONLY=true
            ;;
        -h|--help)
            if [ "$IS_TR" = true ]; then
                echo "Kullanım: $0 [SEÇENEKLER]"
                echo "Seçenekler:"
                echo "  -y, --yes      Onay sormadan güncellemeyi doğrudan kur"
                echo "  -c, --check    Sadece güncelleme kontrolü yap, kurulum yapma"
                echo "  -h, --help     Bu yardım mesajını göster"
            else
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  -y, --yes      Install updates automatically without prompt"
                echo "  -c, --check    Only check for updates without installing"
                echo "  -h, --help     Show this help message"
            fi
            exit 0
            ;;
    esac
done

# SemVer comparison helper: returns 0 (true) if $1 > $2
is_newer() {
    python3 -c "
import sys
v1 = [int(x) for x in sys.argv[1].split('.') if x.isdigit()]
v2 = [int(x) for x in sys.argv[2].split('.') if x.isdigit()]
while len(v1) < 3: v1.append(0)
while len(v2) < 3: v2.append(0)
sys.exit(0 if v1 > v2 else 1)
" "$1" "$2"
}

# 1. Detect current version and resolve stale binary shadowing between /usr/local/bin and /usr/bin
CURRENT_VER=""

# If both /usr/bin and /usr/local/bin binaries exist, determine which is newer
if [ -f "/usr/bin/$BIN_NAME" ] && [ -f "/usr/local/bin/$BIN_NAME" ]; then
    V_USR=$("/usr/bin/$BIN_NAME" --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n 1 || echo "0.0.0")
    V_LOCAL=$("/usr/local/bin/$BIN_NAME" --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n 1 || echo "0.0.0")
    if ! is_newer "$V_LOCAL" "$V_USR"; then
        # /usr/bin is newer or equal, so /usr/local/bin is a stale shadow
        CURRENT_VER="$V_USR"
        if [ -w "/usr/local/bin/$BIN_NAME" ]; then
            rm -f "/usr/local/bin/$BIN_NAME" 2>/dev/null || true
        fi
    else
        CURRENT_VER="$V_LOCAL"
    fi
fi

if [ -z "$CURRENT_VER" ] && command -v "$BIN_NAME" >/dev/null 2>&1; then
    CURRENT_VER=$("$BIN_NAME" --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n 1 || true)
fi

if [ -z "$CURRENT_VER" ] && [ -f "VERSION" ]; then
    CURRENT_VER=$(cat VERSION | tr -d '[:space:]')
fi

if [ -z "$CURRENT_VER" ]; then
    CURRENT_VER="0.0.0"
fi

if [ "$IS_TR" = true ]; then
    echo -e "${CYAN}==>${NC} Güncellemeler denetleniyor... (Mevcut sürüm: ${BOLD}v${CURRENT_VER}${NC})"
else
    echo -e "${CYAN}==>${NC} Checking for updates... (Current version: ${BOLD}v${CURRENT_VER}${NC})"
fi

# 2. Query GitHub Releases API
API_URL="https://api.github.com/repos/${REPO}/releases/latest"
RELEASE_JSON=$(curl -fsSL -H "Accept: application/vnd.github.v3+json" "$API_URL" 2>/dev/null || true)

if [ -z "$RELEASE_JSON" ]; then
    if [ "$IS_TR" = true ]; then
        echo -e "${RED}[HATA] GitHub API'ye bağlanılamadı. Lütfen internet bağlantınızı kontrol edin.${NC}" >&2
    else
        echo -e "${RED}[ERROR] Failed to connect to GitHub Releases API. Check your internet connection.${NC}" >&2
    fi
    exit 1
fi

# 3. Detect system package preference
PREF_EXT=".tar.gz"
DISTRO_FAMILY="unknown"

if [ -f /etc/arch-release ] || [ -f /etc/cachyos-release ] || command -v pacman >/dev/null 2>&1; then
    PREF_EXT=".pkg.tar.zst"
    DISTRO_FAMILY="arch"
elif [ -f /etc/debian_version ] || command -v dpkg >/dev/null 2>&1; then
    PREF_EXT=".deb"
    DISTRO_FAMILY="debian"
elif [ -f /etc/fedora-release ] || [ -f /etc/redhat-release ] || command -v dnf >/dev/null 2>&1 || command -v rpm >/dev/null 2>&1; then
    PREF_EXT=".rpm"
    DISTRO_FAMILY="fedora"
fi

# 4. Parse JSON using Python3
PARSED_INFO=$(python3 -c "
import json, sys

try:
    data = json.loads('''$RELEASE_JSON''')
    tag = data.get('tag_name', '').lstrip('v').strip()
    assets = data.get('assets', [])
    pref = sys.argv[1]
    
    chosen_name = ''
    chosen_url = ''
    for a in assets:
        name = a.get('name', '')
        if name.endswith(pref):
            chosen_name = name
            chosen_url = a.get('browser_download_url', '')
            break
            
    # Fallback to any linux package if preferred extension not found
    if not chosen_url:
        for a in assets:
            name = a.get('name', '')
            if name.endswith('.deb') or name.endswith('.pkg.tar.zst') or name.endswith('.rpm'):
                chosen_name = name
                chosen_url = a.get('browser_download_url', '')
                break

    print(f'{tag}|{chosen_name}|{chosen_url}')
except Exception as e:
    print('ERROR||')
" "$PREF_EXT" 2>/dev/null || echo "ERROR||")

IFS='|' read -r LATEST_VER PKG_NAME PKG_URL <<< "$PARSED_INFO"

if [ "$LATEST_VER" = "ERROR" ] || [ -z "$LATEST_VER" ]; then
    if [ "$IS_TR" = true ]; then
        echo -e "${RED}[HATA] Sürüm bilgisi çözümlenemedi.${NC}" >&2
    else
        echo -e "${RED}[ERROR] Could not parse release version information.${NC}" >&2
    fi
    exit 1
fi

if ! is_newer "$LATEST_VER" "$CURRENT_VER"; then
    if [ "$IS_TR" = true ]; then
        echo -e "${GREEN}[✓] ProtectEye zaten en güncel sürümde! (${BOLD}v${CURRENT_VER}${NC})"
    else
        echo -e "${GREEN}[✓] ProtectEye is already up to date! (${BOLD}v${CURRENT_VER}${NC})"
    fi
    exit 0
fi

if [ "$IS_TR" = true ]; then
    echo -e "${YELLOW}==> Yeni sürüm mevcut: ${GREEN}${BOLD}v${LATEST_VER}${NC} (Şu anki: v${CURRENT_VER})"
else
    echo -e "${YELLOW}==> New version available: ${GREEN}${BOLD}v${LATEST_VER}${NC} (Current: v${CURRENT_VER})"
fi

if [ "$CHECK_ONLY" = true ]; then
    exit 0
fi

# Ask for confirmation if not --yes
if [ "$AUTO_YES" != true ]; then
    if [ "$IS_TR" = true ]; then
        read -p "ProtectEye v${LATEST_VER} sürümüne güncellensin mi? [E/h]: " CONFIRM
        CONFIRM=${CONFIRM:-E}
        if [[ ! "$CONFIRM" =~ ^[EeYy]$ ]]; then
            echo -e "${YELLOW}Güncelleme iptal edildi.${NC}"
            exit 0
        fi
    else
        read -p "Update ProtectEye to v${LATEST_VER} now? [Y/n]: " CONFIRM
        CONFIRM=${CONFIRM:-Y}
        if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
            echo -e "${YELLOW}Update cancelled.${NC}"
            exit 0
        fi
    fi
fi

# Determine if ProtectEye is currently running
WAS_RUNNING=false
if pgrep -x "$BIN_NAME" >/dev/null 2>&1; then
    WAS_RUNNING=true
fi

# 6. Download and Install Package
TEMP_DIR="/tmp/protecteye_update_$$"
mkdir -p "$TEMP_DIR"
trap 'rm -rf "$TEMP_DIR"' EXIT

SUDO=""
if [ "$(id -u)" -ne 0 ] && command -v sudo >/dev/null 2>&1; then
    SUDO="sudo"
fi

if [ -n "$PKG_URL" ] && [ -n "$PKG_NAME" ]; then
    DOWNLOAD_PATH="$TEMP_DIR/$PKG_NAME"
    if [ "$IS_TR" = true ]; then
        echo -e "${CYAN}==>${NC} Paket indiriliyor: ${BOLD}$PKG_NAME${NC}..."
    else
        echo -e "${CYAN}==>${NC} Downloading package: ${BOLD}$PKG_NAME${NC}..."
    fi

    curl -# -L -o "$DOWNLOAD_PATH" "$PKG_URL"

    if [ "$IS_TR" = true ]; then
        echo -e "${CYAN}==>${NC} Kurulum uygulanıyor..."
    else
        echo -e "${CYAN}==>${NC} Applying update..."
    fi

    case "$PKG_NAME" in
        *.pkg.tar.zst)
            if [ -n "$SUDO" ] || [ "$(id -u)" -eq 0 ]; then
                $SUDO pacman -U --noconfirm "$DOWNLOAD_PATH"
                # Clean up any stale shadowed binary in /usr/local/bin
                if [ -f "/usr/local/bin/$BIN_NAME" ]; then
                    $SUDO rm -f "/usr/local/bin/$BIN_NAME" 2>/dev/null || true
                fi
            else
                tar -I zstd -xf "$DOWNLOAD_PATH" -C "$HOME/.local/" --strip-components=1 usr/
            fi
            ;;
        *.deb)
            if [ -n "$SUDO" ] || [ "$(id -u)" -eq 0 ]; then
                $SUDO dpkg -i "$DOWNLOAD_PATH" || $SUDO apt-get install -f -y
                if [ -f "/usr/local/bin/$BIN_NAME" ]; then
                    $SUDO rm -f "/usr/local/bin/$BIN_NAME" 2>/dev/null || true
                fi
            else
                dpkg-deb -x "$DOWNLOAD_PATH" "$TEMP_DIR/extracted"
                cp -rf "$TEMP_DIR/extracted/usr/"* "$HOME/.local/"
            fi
            ;;
        *.rpm)
            if command -v dnf >/dev/null 2>&1; then
                $SUDO dnf install -y "$DOWNLOAD_PATH"
            else
                $SUDO rpm -Uvh "$DOWNLOAD_PATH"
            fi
            if [ -f "/usr/local/bin/$BIN_NAME" ]; then
                $SUDO rm -f "/usr/local/bin/$BIN_NAME" 2>/dev/null || true
            fi
            ;;
        *)
            echo -e "${YELLOW}[!] Bilinmeyen paket formatı, evrensel yükleyiciye geçiliyor...${NC}"
            curl -fsSL https://raw.githubusercontent.com/${REPO}/main/installer.sh | bash -s -- -y
            ;;
    esac
else
    # Fallback: run universal installer
    if [ "$IS_TR" = true ]; then
        echo -e "${CYAN}==>${NC} Evrensel kurulum sihirbazı çalıştırılıyor..."
    else
        echo -e "${CYAN}==>${NC} Running universal installer..."
    fi
    curl -fsSL https://raw.githubusercontent.com/${REPO}/main/installer.sh | bash -s -- -y
fi

# Clean up stale shadow binaries if /usr/bin is now present
if [ -f "/usr/bin/$BIN_NAME" ] && [ -f "/usr/local/bin/$BIN_NAME" ]; then
    if [ -n "$SUDO" ] || [ "$(id -u)" -eq 0 ]; then
        $SUDO rm -f "/usr/local/bin/$BIN_NAME" 2>/dev/null || true
    fi
fi

# 7. Restart application if it was previously running
if [ "$WAS_RUNNING" = true ]; then
    if [ "$IS_TR" = true ]; then
        echo -e "${CYAN}==>${NC} ProtectEye yeni sürümle yeniden başlatılıyor..."
    else
        echo -e "${CYAN}==>${NC} Restarting ProtectEye with updated version..."
    fi
    CURRENT_PID=$$
    PARENT_PID=$PPID
    GRANDPARENT_PID=$(ps -o ppid= -p "$PARENT_PID" 2>/dev/null | tr -d ' ' || true)
    for pid in $(pgrep -x "$BIN_NAME" 2>/dev/null || true); do
        if [ "$pid" -ne "$CURRENT_PID" ] && [ "$pid" -ne "$PARENT_PID" ] && [ "$pid" -ne "$GRANDPARENT_PID" ]; then
            kill "$pid" 2>/dev/null || true
        fi
    done
    sleep 0.5
    nohup "$BIN_NAME" >/dev/null 2>&1 &
fi

if [ "$IS_TR" = true ]; then
    echo -e "\n${GREEN}${BOLD}[✓] ProtectEye başarıyla v${LATEST_VER} sürümüne güncellendi!${NC}\n"
else
    echo -e "\n${GREEN}${BOLD}[✓] ProtectEye has been successfully updated to v${LATEST_VER}!${NC}\n"
fi
