#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
namespace tetris::serveur {
class Player;
class PlayersManager {
    std::mutex mutex;
    std::unordered_map<std::string, std::shared_ptr<Player>> players;

  public:
    bool player_exist(const std::string &name);
    bool add_player(const std::shared_ptr<Player> &player);
    void del_player(const std::string &name, const Player *expected = nullptr);
    std::shared_ptr<Player> get_object(const std::string &name);
};
} // namespace tetris::serveur
