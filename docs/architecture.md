# Architecture

```
src/
  shared/           Packets, bounded transport, game rules and board model
  client/
    network/        Socket lifecycle, authentication and LAN discovery
    gui/            SFML rendering, menu, room and game views
  server/
    gestion_serveur/ Listener, sessions, rooms and their ownership
    game/           One authoritative match loop, four small mode wrappers
    database/       Shared account/score/friend service
 tests/             Game, transport, storage and multi-client regressions
```

## Ownership and lifetime

The server listener owns session workers. Each session has one socket, one packet reader, a bounded input queue and a send mutex. Session removal is conditional on object identity, so a rejected duplicate login cannot remove the original player. Closing a session shuts down its descriptor first; final destruction removes its parser state and closes the descriptor once.

Rooms hold participant names rather than owning session objects. The room manager starts and joins match workers, and reaps completed workers on the next match launch. Room membership, owner changes and start validation are protected by the room mutex. A game can only be started by its owner, with the correct mode and enough players.

Each match owns its game boards and updates them in one thread. Player workers enqueue inputs; they never modify those boards directly. Disconnected or departed sessions are excluded. Remaining players receive a final game-end packet. The GUI returns them to mode selection and leaves the finished room.

The GUI has one active packet reader on its render thread: the room view while waiting, then the running game view. No network thread joins itself or steals another view's packets. Slow authentication and menu requests have finite deadlines; game packet polling returns promptly when a complete packet is not available.

All sessions share one database service. Its mutex covers the complete read/change/save operation, preventing stale per-player snapshots from overwriting other accounts. Password verification uses a random 16-byte salt, PBKDF2-HMAC-SHA256 (210,000 iterations), a 32-byte derived key and a constant-time comparison. This is still a local demo without TLS or production authentication controls.

## Protocol

The refactored client and server must be built from the same snapshot. Old executables use a different encoding and are not compatible.

A packet begins with a 32-bit big-endian direction-specific ID, followed by its typed payload. Integers/enums are 32-bit on the supported build; booleans are one byte (`0` or `1`). String/vector lengths are unsigned 64-bit big-endian. Friend/request/score-list JSON payloads use a 32-bit byte length instead. Room information includes the confirmed count, name, status, mode and owner. Grid data is checked as a 20×10 board.

Serialization first builds a bounded packet buffer, then sends it under the session/client mutex. TCP is a byte stream: a parser retains incomplete bytes and consumes exactly one complete packet at a time. Polling leaves prefixes intact. Unknown IDs, invalid values and oversized lengths raise protocol errors and close that session.

Limits: 64 KiB per string/JSON body, 4,096 elements per vector, 1 MiB per packet buffer, 128 queued inputs, 16 queued effects and 64 simultaneous sessions. Authentication and partial packet reads have a five-second deadline; idle authenticated sessions remain connected. Writes have a 200 ms deadline for each complete encoded packet.

The server listens on localhost unless explicitly configured for a LAN. UDP discovery is a convenience broadcast, not authenticated server identity. No TLS, replay prevention, Internet matchmaking or automatic in-game reconnect is provided. A disconnected game ends; log in again to start a new one.
