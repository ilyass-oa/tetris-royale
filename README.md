# Tetris Royale

A C++ client–server Tetris game with an SFML graphical client, local solo play and competitive multiplayer on a trusted local network.

This started as a **collective university project**. This repository is a maintained, privacy-cleaned snapshot with a new history. Original collaborators' names, student identifiers, reports and account files are deliberately omitted. The subsequent networking fixes, refactoring, tests and documentation were developed with AI assistance. This is not a claim of sole authorship of the original game.

## What you can run

- **Solo / Endless**: local, offline play (`--solo` bypasses the account menu).
- **Classic**: 2–9 players; clearing multiple lines adds garbage to another board.
- **Duel**: exactly 2 players, using the competitive Classic rules.
- **Royal Competition**: 2–9 players, with energy earned by clearing lines and spent on effects.
- Local accounts, friend requests and saved high scores.

Linux is the supported and tested platform. The socket layer uses POSIX APIs; a Windows port is not provided. macOS is not validated.

![Nine-player Classic match, tested with synthetic local clients](docs/screenshots/classic-nine-players.png)

## Build

Requirements: CMake 3.22+, a C++20 compiler, Git, OpenSSL development headers, and the SFML graphics/window dependencies. nlohmann/json is included as a licensed header fallback. CMake uses an installed SFML 3 package when available, otherwise downloads the pinned **SFML 3.0.0** source from its official repository. This first GUI configuration therefore needs Internet access.

On Ubuntu 24.04 / Debian with sufficiently recent CMake and GCC:

```sh
sudo apt install build-essential cmake git libssl-dev libfreetype-dev \
    libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev libgl1-mesa-dev
cmake --preset desktop
cmake --build --preset desktop -j 4
ctest --preset desktop
```

The SFML dependency list comes from the [official SFML build instructions](https://www.sfml-dev.org/tutorials/3.0/getting-started/build-from-source/). Audio and SFML's own networking module are disabled; this project uses its own TCP protocol.

For the server and automated tests without a display or SFML:

```sh
cmake --preset headless
cmake --build --preset headless -j 4
ctest --preset headless
```

Manual equivalent:

```sh
cmake -S . -B build/headless -DTETRIS_BUILD_GUI=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build/headless -j 4
ctest --test-dir build/headless --output-on-failure
```

Executables are in `build/<preset>/src/server/server` and `build/desktop/src/client/gui/TetrisRoyale`. Run from the repository root. Assets are located through the configured source path; after moving the checkout, configure and build it again. Prebuilt binaries are intentionally not committed.

## Play on one computer

Terminal 1:

```sh
./build/desktop/src/server/server --data data/Users.json
```

Terminal 2 (repeat for another player):

```sh
./build/desktop/src/client/gui/TetrisRoyale --server 127.0.0.1
```

Create different demo accounts in each client. Choose the same multiplayer room; the host starts once at least two players have joined. The server confirms the room's actual mode and owner to every client. Duplicate logins are rejected while the original session stays connected.

Usernames accept 1–32 ASCII letters, digits, `_` and `-` (the current GUI field limits input to 15). Passwords accept 1–128 bytes. Use a password unique to this demo: **TCP traffic is not encrypted**. Password hashes on disk do not protect credentials in transit.

Offline solo:

```sh
./build/desktop/src/client/gui/TetrisRoyale --solo
```

Controls: arrow keys to move; **Up** or **Z/X/C** to rotate counterclockwise/clockwise/180°; **Space** to hard drop; **Q** to return to the menu. In Royal Competition, click a player grid to open the effect menu. Board index `0` represents yourself; other board indices are assigned by the server for the match.

## Play on a trusted LAN

The server defaults to loopback. To accept other computers explicitly:

```sh
./build/desktop/src/server/server --bind 0.0.0.0 --data data/Users.json
./build/desktop/src/client/gui/TetrisRoyale --server 192.168.1.10
```

Replace `192.168.1.10` with the server's LAN IPv4 address. Allow **TCP 46587**, and **UDP 46588** only if using discovery, through the host's firewall for your trusted subnet. Keep the firewall enabled.

Optional UDP broadcast discovery:

```sh
TETRIS_DISCOVER=1 ./build/desktop/src/client/gui/TetrisRoyale
```

Discovery waits up to one second, then falls back to localhost. It is optional and may not cross VLANs, guest Wi-Fi isolation or broadcast filters. An explicit `--server` address bypasses it. For custom ports, pass `--port 47000` to both server and client. `server --no-discovery` disables the UDP listener; `server --help` lists the remaining options. Hostnames and IPv6 are not supported.

## Data and privacy

The server creates the account database at the path passed to `--data` (default `data/Users.json`). It stores salted PBKDF2-HMAC-SHA256 password hashes, scores and friend relationships, and restricts the JSON file to mode `0600`. Updates are serialized within one server and replace the file after a complete write.

Use one server process per database. This JSON store is intended for a local demo, not a production account service. Old plaintext databases are refused without overwriting them: keep any existing private backup and choose a fresh path. No real account data or automatic migration is included.

Generated databases, build directories and local configuration are ignored by Git. Never add an account file, an academic report with identifying information, or someone else's personal details to a contribution. Dependency license notices remain intact.

## Development and verification

```sh
cmake --preset sanitize
cmake --build --preset sanitize -j 4
ctest --preset sanitize
```

Tests cover game boundaries/reset/line clearing, fragmented and coalesced TCP packets, invalid and oversized payloads, timeouts, peer closure, account persistence, simultaneous updates, authentication retry, duplicate sessions, room ownership/capacity, all four server modes and disconnects during a match. A GUI smoke test renders the menu, solo and three multiplayer views with actual clients and checks return after match end. It runs automatically when `xvfb-run` is available; on Ubuntu install `xvfb xauth` for that check.

These are regression checks, not a guarantee that every possible bug is gone. See [verification and limits](docs/verification.md), [architecture](docs/architecture.md) and [network troubleshooting](docs/networking.md).

## Origin and licenses

The original academic implementation was collective. Removing personal credits protects collaborators' privacy; it does not erase their contribution. No blanket open-source license has been newly assigned to that shared work. [Public visibility alone does not grant unrestricted reuse rights](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository).

Third-party notices are listed in [THIRD_PARTY.md](THIRD_PARTY.md). DejaVu Sans replaces the old unverified font files. SFML is fetched from its official source rather than redistributing the old platform SDKs.
