# fortuna-leaderboard-server

A lightweight C++ leaderboard server built with the [Drogon](https://github.com/drogonframework/drogon) framework and SQLite storage.

## Features

- **POST /api/v1/scores** – submit a score
- **GET /api/v1/scores?limit=10** – fetch top scores as JSON
- **GET /** – browser-friendly leaderboard that auto-refreshes every 5 seconds
- **GET /scores.txt** – game-compatible plaintext format (`name score sail_color` per line)
- **GET /health** – health check; returns `{"status":"ok"}` with HTTP 200

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

On startup, the server prints the effective address, port, and document root:

```
[main] Starting Fortuna Leaderboard Server
[main] Listening on 0.0.0.0:8080
[main] Document root : ./public/
[main] Health check  : http://0.0.0.0:8080/health
```

### Overriding address / port at runtime

You can override the listen address and port with environment variables without editing `config.json`:

```bash
LISTEN_PORT=9090 ./build/fortuna-leaderboard-server
LISTEN_ADDR=0.0.0.0 LISTEN_PORT=8080 ./build/fortuna-leaderboard-server
```

> **Important:** Always bind to `0.0.0.0` (all interfaces) rather than a specific public IP.  
> Binding to your server's public IP (e.g. `203.0.113.5`) is fragile – it breaks on IP changes  
> and is unnecessary because `0.0.0.0` already accepts connections on every interface.

### Quick connectivity tests

Once the server is running, verify it responds:

```bash
# From the same machine (loopback)
curl http://localhost:8080/health

# From the same machine (all-interfaces binding check)
curl http://0.0.0.0:8080/health

# From a remote machine or your laptop (replace with your server's IP)
curl http://YOUR_SERVER_IP:8080/health
```

A successful response looks like:

```json
{"status":"ok"}
```

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

### GET /health

Returns `{"status":"ok"}` with HTTP 200. Use this endpoint to verify the server is reachable:

```bash
curl -s http://YOUR_SERVER_IP:8080/health
```

## VPS deployment

1. Copy the binary, `config.json`, `sql/`, and `public/` to the server.
2. Point `config.json` → `"filename"` to an absolute path (e.g. `/var/lib/fortuna/leaderboard.db`).
3. Ensure `config.json` listens on `0.0.0.0` (already the default – **do not change to your public IP**).
4. Run the binary as a systemd service:

```ini
[Unit]
Description=Fortuna Leaderboard Server
After=network.target

[Service]
WorkingDirectory=/opt/fortuna-leaderboard-server
ExecStart=/opt/fortuna-leaderboard-server/fortuna-leaderboard-server
Restart=on-failure
User=fortuna

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl enable --now fortuna-leaderboard-server
```

5. Expose port 8080 (or put Nginx/Caddy in front for TLS).

## Troubleshooting – cannot reach the server from the internet

Work through these checks in order.

### 1. Server is not running or crashed

```bash
# Check whether the process is running
pgrep -a fortuna-leaderboard-server

# If using systemd
sudo systemctl status fortuna-leaderboard-server
sudo journalctl -u fortuna-leaderboard-server -n 50 --no-pager
```

Then verify it answers locally (same machine):

```bash
curl http://localhost:8080/health
```

If this fails, the server itself has a problem (check logs, DB path, file permissions).

### 2. Server is bound to localhost or a specific IP instead of 0.0.0.0

Check what the process is actually listening on:

```bash
ss -tlnp | grep 8080
# or
netstat -tlnp | grep 8080
```

You must see `0.0.0.0:8080` (or `:::8080` for IPv6 dual-stack).  
If you see `127.0.0.1:8080`, the server only accepts local connections.

Fix: ensure `config.json` has:

```json
"listeners": [{ "address": "0.0.0.0", "port": 8080, "https": false }]
```

or export `LISTEN_ADDR=0.0.0.0` before starting the server.

### 3. Host firewall (ufw / iptables)

```bash
# ufw
sudo ufw status
sudo ufw allow 8080/tcp
sudo ufw reload

# iptables (allow inbound on port 8080)
sudo iptables -I INPUT -p tcp --dport 8080 -j ACCEPT
```

### 4. Cloud / VPS firewall (IONOS, AWS, DigitalOcean, etc.)

Most VPS providers have a **network-level firewall** that is separate from the OS firewall.

- **IONOS Cloud Panel** → *Network* → *Firewall* → add an inbound rule for TCP port 8080.
- **AWS** → Security Group → add inbound rule TCP 8080 from `0.0.0.0/0`.
- **DigitalOcean** → Networking → Firewalls → add inbound TCP 8080.

After allowing the port in the panel, re-run:

```bash
curl http://YOUR_SERVER_IP:8080/health
```

### 5. SELinux blocking the port

If the host runs SELinux (common on CentOS/RHEL):

```bash
# Check SELinux status
getenforce

# Allow the custom port
sudo semanage port -a -t http_port_t -p tcp 8080
```

### 6. Running inside Docker

If the server runs in a container, you must publish the port:

```bash
docker run -p 8080:8080 your-image
```

And the server inside the container must bind to `0.0.0.0`, not `127.0.0.1`.

### 7. Reverse proxy (Nginx / Caddy) misconfiguration

If you put Nginx in front, make sure it proxies to `127.0.0.1:8080` and that the Nginx port (80/443) is open rather than 8080.

```nginx
server {
    listen 80;
    server_name example.com;
    location / {
        proxy_pass http://127.0.0.1:8080;
    }
}
```

### 8. Quick remote diagnostic checklist

```bash
# From your laptop / another machine:

# 1. Is the port open at the network level?
nc -zv YOUR_SERVER_IP 8080

# 2. Does the server respond to HTTP?
curl -v http://YOUR_SERVER_IP:8080/health

# 3. Trace the route to the server
traceroute YOUR_SERVER_IP
```

