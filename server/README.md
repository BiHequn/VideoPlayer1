# VideoPlayer1 services

The desktop client uses two services on the same host:

- FastAPI authentication service, port `8000` by default.
- C media service, port `8888` by default.

## Configuration

Copy the example configuration and generate a private shared token secret:

```bash
cp .env.example .env
openssl rand -hex 32
openssl rand -hex 32
```

Generate two different values and put them in `VIDEOPLAYER_TOKEN_SECRET` and
`VIDEOPLAYER_REFRESH_TOKEN_SECRET` inside `.env`. The access-token secret is
loaded by both services; each secret must contain at least 32 characters.
Never commit `.env`.

## Start and stop

Install Python 3.10 or newer and the C dependencies, then run:

```bash
chmod +x scripts/*.sh
scripts/start.sh
scripts/status.sh
scripts/stop.sh
```

The scripts write process IDs to `run/` and logs to `logs/`. To select a
specific Python interpreter, set `VIDEOPLAYER_PYTHON`, for example:

```bash
VIDEOPLAYER_PYTHON=/path/to/venv/bin/python scripts/start.sh
```
