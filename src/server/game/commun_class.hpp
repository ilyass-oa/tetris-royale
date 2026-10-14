#pragma once
#include "../gestion_serveur/room.hpp"
namespace tetris::serveur {
// A single game loop owns all boards, timers and effects for a match.
void run_match(Room &room, const std::shared_ptr<PlayersManager> &players, GameMode mode);
} // namespace tetris::serveur
