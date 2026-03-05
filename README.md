# fortuna-leaderboard-server

A lightweight C++ leaderboard server built with the [Drogon](https://github.com/drogonframework/drogon) framework and SQLite storage.

## Features

- **POST /api/v1/scores** – submit a score
- **GET /api/v1/scores?limit=10** – fetch top scores as JSON
- **GET /** – browser-friendly leaderboard that auto-refreshes every 5 seconds
- **GET /scores.txt** – game-compatible plaintext format (`name score sail_color` per line)
- **GET /health** – liveness probe; returns `{"ok":true,"status":"healthy"}`

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

### GET /health

Returns `200 OK` with `{"ok":true,"status":"healthy"}` and no-cache headers.
Useful as a liveness probe or quick smoke-test from the browser.

## Browser access & troubleshooting

### Accessing the leaderboard via raw IP:port (no HTTPS)

The server speaks plain HTTP on port 8080.  Open it in a browser with an
explicit `http://` scheme and the IP address of the host:

```
http://203.0.113.42:8080/
```

> **Important – always type the full URL including `http://`.**  
> If you omit the scheme many browsers default to `https://`, which will fail
> because the server does not terminate TLS.

For a local machine use `http://127.0.0.1:8080/` (not `localhost` – see the
IPv6 note below).

### Why browsers upgrade HTTP to HTTPS automatically

Modern browsers enforce **HTTP Strict Transport Security (HSTS)**.  If a
hostname was ever visited over HTTPS (or is included in the browser's
built-in preload list) the browser will silently rewrite every subsequent
`http://` request for that hostname to `https://`, causing an immediate
connection failure.

HSTS **does not apply to bare IP addresses**.  Accessing the server via its
raw IP (e.g. `http://203.0.113.42:8080/`) bypasses this entirely.

#### Diagnosing HSTS in Chrome / Edge

1. Navigate to `chrome://net-internals/#hsts`.
2. In the **Query HSTS/PKP domain** box enter the hostname you are trying to
   reach.
3. If the output shows `found: true` with `sts_include_subdomains` or an
   expiry in the future, HSTS is enforced.
4. Use **Delete domain security policies** on the same page to clear the
   entry, then retry with the explicit `http://` URL.

#### Diagnosing HSTS in Firefox

1. Open `about:support` → *Profile Folder* → open in explorer.
2. Delete (or rename) `SiteSecurityServiceState.bin` / `SiteSecurityServiceState.txt`.
3. Restart Firefox.

### IPv6 access

If the server is also bound to an IPv6 interface, wrap the address in square
brackets in the URL:

```
http://[2001:db8::1]:8080/
```

For loopback: `http://[::1]:8080/`

Some common pitfalls:

| Symptom | Likely cause |
|---------|-------------|
| Browser refuses connection on `localhost` | Browser resolves `localhost` to `::1` (IPv6) and the server only listens on `0.0.0.0` (IPv4). Use `127.0.0.1` instead, or add an IPv6 listener in `config.json`. |
| `ERR_CONNECTION_REFUSED` on the IP but the service is running | Firewall / `ufw` is blocking port 8080. Run `sudo ufw allow 8080/tcp`. |
| Page loads over HTTP but `fetch()` inside the page fails | Browser mixed-content policy blocks HTTP sub-requests from an HTTPS page. Serve both the page and the API over plain HTTP. |

### Adding an IPv6 listener

Edit `config.json` and add a second listener entry:

```json
"listeners": [
    { "address": "0.0.0.0", "port": 8080, "https": false },
    { "address": "::",       "port": 8080, "https": false }
]
```

### Verifying the server is reachable

Use `curl` to bypass the browser entirely:

```bash
# Basic connectivity + health check
curl -v http://127.0.0.1:8080/health

# Check response headers for Cache-Control
curl -I http://127.0.0.1:8080/api/v1/scores
```

All API endpoints return `Cache-Control: no-store, no-cache, must-revalidate`
so intermediate proxies and browsers will not serve stale data.

## VPS deployment

1. Copy the binary, `config.json`, `sql/`, and `public/` to the server.
2. Point `config.json` → `"filename"` to an absolute path (e.g. `/var/lib/fortuna/leaderboard.db`).
3. Run the binary as a systemd service:

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

4. Expose port 8080 (or put Nginx/Caddy in front for TLS).
