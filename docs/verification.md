# Verification and remaining limits

Verified on 2026-10-14, on Linux with GCC 16.1.1, CMake 4.4.1, OpenSSL 3.6.3 and SFML 3.0.0.

The build used SFML from its official source and the included nlohmann/json fallback (`CMAKE_DISABLE_FIND_PACKAGE_nlohmann_json=ON`).

## Checks performed

| Check | Result | Scope |
| --- | --- | --- |
| Desktop build and CTest | 5/5 passed | Game, transport, database, integration, graphical smoke |
| ASan + UBSan headless build and CTest | 4/4 passed; no sanitizer errors reported | Game, transport, database, integration |
| GUI with Xvfb | Passed | Menu, offline solo, Classic with nine real local clients, Duel, Royal, match exit and disconnected send |
| Server CLI | Passed | Occupied TCP port reports a bind error; SIGTERM exits cleanly |

The sanitizer run checks the server, transport and game model; it does not claim to sanitize SFML or the GUI. GUI screenshots were inspected separately. Generated accounts in all tests are synthetic and temporary.

## Regression cases

- Fragmented and coalesced packets, invalid IDs/booleans/lengths, bounded reads/writes and peer closure without SIGPIPE.
- Final packet bytes delivered when `POLLIN` and `POLLHUP` arrive together; EOF observed afterwards.
- Room mode/owner, effect/target fields, JSON response IDs and score payloads survive packet round trips.
- Authentication failure followed by retry, duplicate live login without removing the original session, room capacity and ownership handover.
- Four server game modes, multiple consecutive matches, malformed peers, departure/disconnection during a match and bounded shutdown.
- Password salting, file permissions, persistent best scores, friend request validation and simultaneous database updates. Clients cannot overwrite saved scores with forged score packets.
- Board bounds, cyan occupied cells, line movement, clamped garbage, reset and repeated rotations/drops until top-out. A red tetromino and a blackout board do not falsely mark a Royal player as eliminated.
- One active GUI reader, actual nine-player rendering including board index 9, return after match end, and safe sending after disconnect.

## What is not established

These checks do not cover every possible failure.

- Actual multi-computer Wi-Fi/LAN sessions and UDP discovery across different routers were not tested. Use an explicit IPv4 address first.
- Ubuntu GitHub Actions is configured separately; local Linux results do not substitute for its remote results.
- Windows is unsupported and macOS is unvalidated.
- Multiplayer traffic has no TLS. Use synthetic accounts and a trusted network, not an Internet-facing account service.
- JSON persistence supports one server process per database; power-loss durability and multi-process locking are not provided.
- A disconnected match cannot be resumed; reconnect and start a new one.
- Tournament balance, long-running soak tests and every screen size have not been validated. Existing unused GUI helpers and compiler warnings are not evidence of production readiness.

See [architecture](architecture.md) and [network troubleshooting](networking.md) for the operating assumptions.
