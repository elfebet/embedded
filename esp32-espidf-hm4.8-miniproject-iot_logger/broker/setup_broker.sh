#!/usr/bin/env bash
#
# Встановлює (якщо потрібно) і запускає MQTT-брокер Mosquitto для проєкту
# iot_logger (Крок 8, Частина 1), одразу з двома listener'ами:
#   1883/tcp        -- прошивка (mqtt_link.c)
#   9001/websockets -- веб-консоль (web/templates/html/dashboard.html, Челендж 7)
#
# Використання:
#   ./setup_broker.sh                # встановити (якщо треба) і запустити
#   ./setup_broker.sh --no-install    # лише запустити, не чіпати встановлення
#
# Підтримує Windows (Git Bash, через winget), Linux (apt) і macOS (brew) --
# ті самі три шляхи встановлення, що описані в Кроці 8, Частина 1, лише
# автоматизовані в один виклик.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONF="$SCRIPT_DIR/mosquitto.conf"
DO_INSTALL=1

for arg in "$@"; do
    case "$arg" in
        --no-install) DO_INSTALL=0 ;;
        -h|--help)
            echo "Usage: $0 [--no-install]"
            exit 0
            ;;
    esac
done

log()  { printf '==> %s\n' "$1"; }
warn() { printf 'УВАГА: %s\n' "$1" >&2; }
die()  { printf 'ПОМИЛКА: %s\n' "$1" >&2; exit 1; }

[ -f "$CONF" ] || die "не знайдено $CONF (очікую поруч зі скриптом)"

OS="$(uname -s)"

is_windows() { [[ "$OS" == MINGW* || "$OS" == MSYS* || "$OS" == CYGWIN* ]]; }

install_broker() {
    if is_windows; then
        if [ -f "/c/Program Files/mosquitto/mosquitto.exe" ]; then
            log "Mosquitto вже встановлений -- пропускаю"
            return
        fi
        command -v winget.exe >/dev/null 2>&1 || command -v winget >/dev/null 2>&1 \
            || die "winget не знайдено -- встанови Mosquitto вручну: https://mosquitto.org/download/"
        log "Встановлюю Mosquitto через winget (Windows)..."
        powershell.exe -NoProfile -Command \
            "winget install --id EclipseFoundation.Mosquitto -e --accept-package-agreements --accept-source-agreements --silent" \
            || die "winget install не вдався -- встанови вручну: https://mosquitto.org/download/"
    elif [ "$OS" = "Linux" ]; then
        command -v mosquitto >/dev/null 2>&1 && { log "Mosquitto вже встановлений -- пропускаю"; return; }
        log "Встановлюю Mosquitto через apt (Linux/Debian/Ubuntu)..."
        sudo apt update && sudo apt install -y mosquitto mosquitto-clients
    elif [ "$OS" = "Darwin" ]; then
        command -v mosquitto >/dev/null 2>&1 && { log "Mosquitto вже встановлений -- пропускаю"; return; }
        command -v brew >/dev/null 2>&1 || die "Homebrew не знайдено -- встанови з https://brew.sh або Mosquitto вручну"
        log "Встановлюю Mosquitto через brew (macOS)..."
        brew install mosquitto
    else
        die "Невідома ОС ($OS) -- встанови Mosquitto вручну: https://mosquitto.org/download/"
    fi
}

detect_lan_ip() {
    local ip=""
    if is_windows; then
        ip="$(ipconfig.exe 2>/dev/null | grep -A6 'Wireless LAN adapter Wi-Fi' \
              | grep 'IPv4 Address' | head -1 | sed -E 's/.*: *([0-9.]+).*/\1/')"
        [ -z "$ip" ] && ip="$(ipconfig.exe 2>/dev/null | grep 'IPv4 Address' \
              | grep -v '169\.254\.' | head -1 | sed -E 's/.*: *([0-9.]+).*/\1/')"
    elif [ "$OS" = "Linux" ]; then
        ip="$(ip -4 addr show scope global 2>/dev/null | grep inet | head -1 | awk '{print $2}' | cut -d/ -f1)"
    elif [ "$OS" = "Darwin" ]; then
        ip="$(ipconfig getifaddr en0 2>/dev/null || ipconfig getifaddr en1 2>/dev/null || true)"
    fi
    echo "$ip"
}

if [ "$DO_INSTALL" -eq 1 ]; then
    install_broker
else
    log "--no-install: пропускаю крок встановлення"
fi

LAN_IP="$(detect_lan_ip || true)"
if [ -n "$LAN_IP" ]; then
    log "IP цього комп'ютера в локальній мережі: $LAN_IP"
    log "  -> у idf.py menuconfig впиши:      mqtt://$LAN_IP:1883"
    log "  -> у web/templates/html/dashboard.html впиши: ws://$LAN_IP:9001"
else
    warn "Не вдалось автоматично визначити IP -- знайди його вручну (ipconfig / ip addr / ifconfig)"
fi

log "Запускаю Mosquitto з $CONF (Ctrl+C -- зупинити)..."
if is_windows; then
    exec "/c/Program Files/mosquitto/mosquitto.exe" -c "$(cygpath -w "$CONF")" -v
else
    exec mosquitto -c "$CONF" -v
fi
