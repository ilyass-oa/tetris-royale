#include "classique.hpp"
#include "commun_class.hpp"
namespace tetris::serveur {
Classique::Classique(std::string owner, std::string name, std::shared_ptr<PlayersManager> players,
                     std::shared_ptr<RoomsManager> rooms)
    : Room(9, std::move(owner), std::move(name), std::move(players), std::move(rooms),
           GameMode::Classique) {}
void Classique::run() {
    run_match(*this, players, GameMode::Classique);
}
} // namespace tetris::serveur
