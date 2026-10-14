#include "endless.hpp"
#include "commun_class.hpp"
namespace tetris::serveur {
Endless::Endless(std::string owner, std::string name, std::shared_ptr<PlayersManager> players,
                 std::shared_ptr<RoomsManager> rooms)
    : Room(1, std::move(owner), std::move(name), std::move(players), std::move(rooms),
           GameMode::Endless) {}
void Endless::run() {
    run_match(*this, players, GameMode::Endless);
}
} // namespace tetris::serveur
