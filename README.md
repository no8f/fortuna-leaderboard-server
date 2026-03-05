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
