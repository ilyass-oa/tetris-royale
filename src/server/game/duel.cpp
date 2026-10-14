#include "duel.hpp"
#include "commun_class.hpp"
namespace tetris::serveur {
Duel::Duel(std::string owner, std::string name, std::shared_ptr<PlayersManager> players,
           std::shared_ptr<RoomsManager> rooms)
    : Room(2, std::move(owner), std::move(name), std::move(players), std::move(rooms),
           GameMode::Duel) {}
void Duel::run() {
    run_match(*this, players, GameMode::Duel);
}
} // namespace tetris::serveur
