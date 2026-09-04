#!/usr/bin/env bash
# install.sh — install zion-client as a systemd service that starts at boot.
#
# Usage:
#   sudo ./systemd/install.sh              # auto-detect the built binary
#   sudo ./systemd/install.sh PATH/TO/zion-client
#   sudo ./systemd/install.sh --no-start   # install + enable, but don't start now
#
# What it does:
#   1. Creates the system user 'zion' (no login shell).
#   2. Installs the binary to /usr/bin/zion-client.
#   3. Creates /etc/zion/client.conf from the sample if not already present.
#   4. Creates /var/lib/zion (spool + working dir) owned by zion.
#   5. Installs systemd/zion-client.service to /etc/systemd/system/.
#   6. daemon-reload, enables the unit at boot, and starts it.

set -Eeuo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SERVICE_NAME="zion-client"
SERVICE_USER="zion"
BINARY_SRC="${1:-}"
START_NOW=1

if [[ "${1:-}" == "--no-start" ]]; then
    START_NOW=0
    BINARY_SRC="${2:-}"
fi

die() { printf 'install.sh: %s\n' "$*" >&2; exit 1; }
note() { printf 'install.sh: %s\n' "$*"; }

[[ $EUID -eq 0 ]] || die "must run as root (use sudo)"
command -v systemctl >/dev/null 2>&1 || die "systemctl not found — this system does not use systemd"

# ── 1. Locate the built binary ────────────────────────────────────────────────
if [[ -z "$BINARY_SRC" ]]; then
    for candidate in \
        "$REPO_DIR/build/cross/bin/zion-client" \
        "$REPO_DIR/build/bin/zion-client"; do
        if [[ -x "$candidate" ]]; then
            BINARY_SRC="$candidate"
            break
        fi
    done
fi
[[ -n "$BINARY_SRC" && -x "$BINARY_SRC" ]] ||
    die "zion-client binary not found. Build first (make build) or pass the path: sudo ./systemd/install.sh build/cross/bin/zion-client"

# ── 2. Service user ───────────────────────────────────────────────────────────
if id -u "$SERVICE_USER" >/dev/null 2>&1; then
    note "user '$SERVICE_USER' already exists"
else
    useradd --system --no-create-home --home-dir /var/lib/zion \
        --shell /usr/sbin/nologin "$SERVICE_USER"
    note "created system user '$SERVICE_USER'"
fi

# ── 3. Binary ─────────────────────────────────────────────────────────────────
install -m 0755 "$BINARY_SRC" /usr/bin/zion-client
note "installed /usr/bin/zion-client from $BINARY_SRC"

# ── 4. Configuration ──────────────────────────────────────────────────────────
mkdir -p /etc/zion
if [[ -f /etc/zion/client.conf ]]; then
    note "/etc/zion/client.conf already exists — leaving it untouched"
else
    install -m 0644 "$REPO_DIR/conf/client.conf.sample" /etc/zion/client.conf
    note "installed /etc/zion/client.conf from sample — edit server_address before use!"
fi

# ── 5. State directory (spool) ────────────────────────────────────────────────
mkdir -p /var/lib/zion/spool
chown -R "$SERVICE_USER:$SERVICE_USER" /var/lib/zion
note "state directory ready at /var/lib/zion"

# ── 6. Unit file, reload, enable at boot ─────────────────────────────────────
install -m 0644 "$REPO_DIR/systemd/zion-client.service" \
    "/etc/systemd/system/${SERVICE_NAME}.service"
systemctl daemon-reload
systemctl enable "$SERVICE_NAME"
note "service enabled — will start automatically at boot"

if [[ $START_NOW -eq 1 ]]; then
    systemctl restart "$SERVICE_NAME"
    sleep 1
    systemctl --no-pager --lines 5 status "$SERVICE_NAME" || true
    note "service started. Follow logs with: journalctl -u ${SERVICE_NAME} -f"
else
    note "installed and enabled (not started). Start with: sudo systemctl start ${SERVICE_NAME}"
fi