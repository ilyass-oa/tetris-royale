#include "br.hpp"
#include "commun_class.hpp"
namespace tetris::serveur {
BR::BR(std::string owner, std::string name, std::shared_ptr<PlayersManager> players,
       std::shared_ptr<RoomsManager> rooms)
    : Room(9, std::move(owner), std::move(name), std::move(players), std::move(rooms),
           GameMode::BR) {}
void BR::run() {
    run_match(*this, players, GameMode::BR);
}
} // namespace tetris::serveur
