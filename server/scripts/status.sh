#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVER_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
RUN_DIR="$SERVER_DIR/run"
EXIT_CODE=0

show_status() {
    local name="$1"
    local pid_file="$2"

    if [[ -f "$pid_file" ]] && kill -0 "$(cat "$pid_file")" 2>/dev/null; then
        echo "$name: running (PID $(cat "$pid_file"))"
    else
        echo "$name: stopped"
        EXIT_CODE=1
    fi
}

show_status "API service" "$RUN_DIR/api.pid"
show_status "Media service" "$RUN_DIR/media.pid"
exit "$EXIT_CODE"
