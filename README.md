# fortuna-leaderboard-server

A lightweight C++ leaderboard server built with the [Drogon](https://github.com/drogonframework/drogon) framework and SQLite storage.

## Features

- **POST /api/v1/scores** – submit a score
- **GET /api/v1/scores?limit=10** – fetch top scores as JSON
- **GET /** – browser-friendly leaderboard that auto-refreshes every 5 seconds
- **GET /scores.txt** – game-compatible plaintext format (`name score sail_color` per line)

## Requirements

| Tool | Minimum version |
|------|----------------|
| CMake | 3.16 (4.x recommended) |
| C++ compiler | GCC 11 / Clang 14 or newer with C++17 |
| Drogon | 1.8 |
| SQLite | 3 |

### Ubuntu/Debian – install all dependencies

```bash
sudo apt-get update
sudo apt-get install -y \
    libdrogon-dev libtrantor-dev \
    libjsoncpp-dev \
    libsqlite3-dev \
    uuid-dev zlib1g-dev libssl-dev \
    libbrotli-dev libhiredis-dev libyaml-cpp-dev \
    libpq-dev default-libmysqlclient-dev
```

### Install CMake 4 (optional – required for `cmake_minimum_required(VERSION … 4.0)`)

```bash
pip install --upgrade cmake
export PATH="$(python3 -c 'import cmake; print(cmake.CMAKE_BIN_DIR)'):$PATH"
```

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The binary is produced at `build/fortuna-leaderboard-server`.

## Running locally

```bash
./build/fortuna-leaderboard-server
```

The server reads `config.json` from the working directory, initialises the SQLite database (`leaderboard.db`), and listens on **http://0.0.0.0:8080**.

## API reference

### POST /api/v1/scores

```json
{ "name": "Alice", "score": 1500, "sail_color": 1 }
```

| Field | Type | Constraints |
|-------|------|-------------|
| `name` | string | 1–15 chars; `^[A-Za-z0-9_-]+$` |
| `score` | integer | ≥ 0 |
| `sail_color` | integer | any integer |

**200 OK** `{ "ok": true }` on success  
**400 Bad Request** `{ "ok": false, "error": "..." }` on validation failure  
**500 Internal Server Error** on DB error

### GET /api/v1/scores?limit=10

`limit` is optional (default 10, clamped 1–100). Returns scores ordered by `score DESC`, then `id ASC`.

```json
{
  "scores": [
    { "name": "Alice", "score": 1500, "sail_color": 1 }
  ]
}
```

### GET /scores.txt

Returns the top 10 scores in game-compatible plaintext:

```
Alice 1500 1
Bob 1200 3
```

## VPS deployment

### 1 – Deploy the application files

```bash
sudo mkdir -p /opt/fortuna-leaderboard-server
sudo cp build/fortuna-leaderboard-server config.json /opt/fortuna-leaderboard-server/
sudo cp -r sql public /opt/fortuna-leaderboard-server/
```

Edit `config.json` to point `"filename"` at an absolute path:

```json
{ "filename": "/opt/fortuna-leaderboard-server/leaderboard.db" }
```

### 2 – Create a dedicated system user

```bash
sudo useradd --system --no-create-home --shell /usr/sbin/nologin fortuna
sudo chown -R fortuna:fortuna /opt/fortuna-leaderboard-server
```

### 3 – Install the systemd unit

A ready-to-use unit file lives at [`deploy/systemd/fortuna-leaderboard.service`](deploy/systemd/fortuna-leaderboard.service).

```bash
sudo cp deploy/systemd/fortuna-leaderboard.service \
         /etc/systemd/system/fortuna-leaderboard.service
sudo systemctl daemon-reload
sudo systemctl enable --now fortuna-leaderboard
```

The server listens on **http://127.0.0.1:8080** (localhost only when behind a proxy).

### 4 – Set up nginx with Let's Encrypt TLS (recommended)

Browsers increasingly refuse plain HTTP connections—see [Troubleshooting](#troubleshooting) below.  
The recommended setup is to run **nginx on port 443** and proxy requests to Drogon on localhost:8080.

#### Install nginx and certbot

```bash
sudo apt install -y nginx certbot python3-certbot-nginx
```

#### Configure the nginx site

A sample config lives at [`deploy/nginx/fortuna-leaderboard.conf`](deploy/nginx/fortuna-leaderboard.conf).

```bash
sudo cp deploy/nginx/fortuna-leaderboard.conf \
         /etc/nginx/sites-available/fortuna-leaderboard
# Edit the two server_name lines to your actual domain
sudo nano /etc/nginx/sites-available/fortuna-leaderboard

sudo ln -s /etc/nginx/sites-available/fortuna-leaderboard \
           /etc/nginx/sites-enabled/fortuna-leaderboard
sudo nginx -t && sudo systemctl reload nginx
```

#### Obtain a TLS certificate

```bash
sudo certbot --nginx -d your.domain.example
```

Certbot patches the nginx config automatically and sets up a cron job for renewal.  
After this, the leaderboard is reachable at `https://your.domain.example`.

#### Key nginx behaviours in the provided config

| Feature | Detail |
|---------|--------|
| HTTP → HTTPS redirect | All port-80 traffic is redirected to 443 |
| HSTS | `Strict-Transport-Security: max-age=31536000` sent on every HTTPS response |
| Proxy headers | `X-Forwarded-For`, `X-Forwarded-Proto`, `X-Real-IP` forwarded to Drogon |
| Keep-alive | `proxy_http_version 1.1` keeps connections open to Drogon |

> **Note:** If you are still testing and not ready for permanent HSTS, reduce  
> `max-age` in the nginx config (e.g. `max-age=300`) before going live.

---

## Troubleshooting

### Browser refuses to open the page / forces HTTPS

Modern browsers apply **HSTS (HTTP Strict Transport Security)** or **HTTPS-Only mode**, which causes them to refuse (or silently upgrade) plain-HTTP connections to a host that was previously visited over HTTPS.

**Symptoms**

- Chrome/Edge shows *"This site can't provide a secure connection"* (`ERR_SSL_PROTOCOL_ERROR`)
- Firefox shows *"Secure Connection Failed"*
- The URL bar shows `https://` even though you typed `http://`

**Solutions (choose one)**

| Situation | Fix |
|-----------|-----|
| Deploying for real users | Set up nginx + Let's Encrypt (see [VPS deployment](#vps-deployment)) |
| Local / LAN testing with a custom domain | Use a self-signed cert or [mkcert](https://github.com/FiloSottile/mkcert) and configure nginx |
| Quick test on `localhost` | `localhost` is exempt from HSTS in most browsers—access via `http://localhost:8080` |
| Chrome HSTS cached for your domain | Navigate to `chrome://net-internals/#hsts`, find the domain, click **Delete** |
| Firefox HSTS cached | Open *Privacy & Security → Certificates → Clear History* or use a private window |

### Port 8080 is not reachable from outside the server

By default the server binds to `0.0.0.0:8080`.  
If you are running nginx as a reverse proxy, restrict Drogon to localhost only by changing `config.json`:

```json
{ "address": "127.0.0.1", "port": 8080, "https": false }
```

Then open only ports 80 and 443 in your firewall:

```bash
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw deny 8080/tcp
```

> **Note:** The JSON snippet above shows only the fields being changed.  
> The full `config.json` structure is shown in step 1 of this section.
