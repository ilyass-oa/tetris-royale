#pragma once
#include "../gestion_serveur/room.hpp"
namespace tetris::serveur {
class BR final : public Room {
  public:
    BR(std::string owner, std::string name, std::shared_ptr<PlayersManager> players,
       std::shared_ptr<RoomsManager> rooms);
    void run() override;
};
} // namespace tetris::serveur
