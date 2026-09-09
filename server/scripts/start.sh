#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVER_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ENV_FILE="${VIDEOPLAYER_ENV_FILE:-$SERVER_DIR/.env}"
RUN_DIR="$SERVER_DIR/run"
LOG_DIR="$SERVER_DIR/logs"

if [[ -f "$ENV_FILE" ]]; then
    set -a
    # shellcheck disable=SC1090
    source "$ENV_FILE"
    set +a
fi

: "${VIDEOPLAYER_TOKEN_SECRET:?Set VIDEOPLAYER_TOKEN_SECRET in $ENV_FILE}"
: "${VIDEOPLAYER_REFRESH_TOKEN_SECRET:?Set VIDEOPLAYER_REFRESH_TOKEN_SECRET in $ENV_FILE}"
if (( ${#VIDEOPLAYER_TOKEN_SECRET} < 32 )); then
    echo "VIDEOPLAYER_TOKEN_SECRET must contain at least 32 characters." >&2
    exit 1
fi
if (( ${#VIDEOPLAYER_REFRESH_TOKEN_SECRET} < 32 )); then
    echo "VIDEOPLAYER_REFRESH_TOKEN_SECRET must contain at least 32 characters." >&2
    exit 1
fi

API_HOST="${VIDEOPLAYER_API_HOST:-0.0.0.0}"
API_PORT="${VIDEOPLAYER_API_PORT:-8000}"
MEDIA_HOST="${VIDEOPLAYER_MEDIA_HOST:-0.0.0.0}"
MEDIA_PORT="${VIDEOPLAYER_MEDIA_PORT:-8888}"
PYTHON_BIN="${VIDEOPLAYER_PYTHON:-}"

if [[ -z "$PYTHON_BIN" ]]; then
    if [[ -x "$SERVER_DIR/.venv/bin/python" ]]; then
        PYTHON_BIN="$SERVER_DIR/.venv/bin/python"
    else
        PYTHON_BIN="python3"
    fi
fi

mkdir -p "$RUN_DIR" "$LOG_DIR"

is_running() {
    local pid_file="$1"
    [[ -f "$pid_file" ]] && kill -0 "$(cat "$pid_file")" 2>/dev/null
}

if is_running "$RUN_DIR/api.pid" || is_running "$RUN_DIR/media.pid"; then
    echo "A VideoPlayer1 service is already running. Use scripts/status.sh." >&2
    exit 1
fi

if ! "$PYTHON_BIN" -c "import sys; raise SystemExit(sys.version_info < (3, 10))"; then
    echo "VideoPlayer1 API requires Python 3.10 or newer." >&2
    exit 1
fi

if ! "$PYTHON_BIN" -c "import uvicorn" >/dev/null 2>&1; then
    echo "uvicorn is unavailable in $PYTHON_BIN. Install server/requirements.txt first." >&2
    exit 1
fi

make -C "$SERVER_DIR"

cd "$SERVER_DIR"
nohup "$PYTHON_BIN" -m uvicorn app.main:app \
    --host "$API_HOST" --port "$API_PORT" \
    > "$LOG_DIR/api.log" 2>&1 < /dev/null &
echo $! > "$RUN_DIR/api.pid"

nohup ./videoserver "$MEDIA_HOST" "$MEDIA_PORT" \
    > "$LOG_DIR/media.log" 2>&1 < /dev/null &
echo $! > "$RUN_DIR/media.pid"

sleep 1
if ! is_running "$RUN_DIR/api.pid" || ! is_running "$RUN_DIR/media.pid"; then
    "$SCRIPT_DIR/stop.sh" >/dev/null 2>&1 || true
    echo "A service failed to start. Check logs/api.log and logs/media.log." >&2
    exit 1
fi

echo "API service:   http://$API_HOST:$API_PORT"
echo "Media service: $MEDIA_HOST:$MEDIA_PORT"
