#pragma once
#include "player_manager.hpp"
#include "room_manager.hpp"
#include <atomic>
#include <game/TetrisGame.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
namespace tetris::serveur {
class Room {
    mutable std::mutex mutex;
    std::string owner, room_name;
    std::unordered_set<std::string> members;
    int capacity;
    GameMode mode;
    bool playing = false;
    std::atomic<bool> cancelled{false};

  protected:
    std::shared_ptr<PlayersManager> players;

  public:
    Room(int capacity, std::string owner, std::string name, std::shared_ptr<PlayersManager> players,
         std::shared_ptr<RoomsManager> rooms, GameMode mode);
    virtual ~Room() = default;
    virtual void run() = 0;
    bool add_player(const std::string &name);
    void del_player(const std::string &name);
    std::string get_name() const;
    std::string get_owner() const;
    bool set_capacity(int requested);
    int get_players_number() const;
    bool is_owner(const std::string &name) const;
    GameMode get_mode() const {
        return mode;
    }
    bool try_start(const std::string &name, GameMode requested);
    void finish();
    bool is_playing() const;
    void cancel() {
        cancelled.store(true);
    }
    bool is_cancelled() const {
        return cancelled.load();
    }
    std::unordered_set<std::string> get_deep_cpy_list(const std::string &except = "") const;
};
} // namespace tetris::serveur
