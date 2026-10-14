#include "player_manager.hpp"
#include "player.hpp"
namespace tetris::serveur {
bool PlayersManager::player_exist(const std::string &name) {
    return bool(get_object(name));
}
bool PlayersManager::add_player(const std::shared_ptr<Player> &player) {
    auto name = player->get_name();
    std::lock_guard lock(mutex);
    return players.emplace(name, player).second;
}
void PlayersManager::del_player(const std::string &name, const Player *expected) {
    std::lock_guard lock(mutex);
    auto it = players.find(name);
    if (it != players.end() && (!expected || it->second.get() == expected))
        players.erase(it);
}
std::shared_ptr<Player> PlayersManager::get_object(const std::string &name) {
    std::lock_guard lock(mutex);
    auto it = players.find(name);
    return it == players.end() ? nullptr : it->second;
}
} // namespace tetris::serveur
