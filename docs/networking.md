# Networking and troubleshooting

| Symptom | Check |
| --- | --- |
| Connection refused | Start the new `server` binary; match `--server` and `--port`. The default server listens only on `127.0.0.1`. |
| Local works, another machine fails | Use `--bind 0.0.0.0` on the host, its actual LAN IPv4 on the client, and a firewall rule allowing TCP 46587 from the trusted subnet. Check guest-network isolation. |
| Discovery fails | Use an explicit IPv4 address. Broadcast is optional and often filtered; UDP 46588 is separate from the TCP game port. |
| Login/signup fails | Keep the server running; use a valid username and a demo-only password. Existing usernames cannot be registered again; use login. A live duplicate session is rejected. The same client can retry after failure. |
| Server reports unsupported database | Do not replace a private old file. Run with a new `--data` path. The public version does not include plaintext migration. |
| GUI returns after disconnection | The match cannot resume an old connection. Log in again and create/join a new room. |
| Room cannot start | Confirm at least two players, correct mode and current owner. Duel has a capacity of two. |
| Missing assets after moving the project | Configure/build again; the binary uses the checkout path configured by CMake. |
| Port is already occupied | Stop your previous demo server or choose a different `--port`. The server exits with a useful bind error. |

Environment equivalents: `TETRIS_SERVER`, `TETRIS_PORT`, optional `TETRIS_DISCOVER=1`, and `TETRIS_CONNECT_TIMEOUT_MS` (positive milliseconds, at most 60,000; default 3,000). An explicit address is preferred when several servers exist.

Do not disable the firewall, expose this demo directly to the Internet, reuse a real password, or mix old and new binaries.
