#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
namespace tetris::serveur {
class Room;
class RoomsManager {
    std::mutex mutex;
    std::unordered_map<std::string, std::shared_ptr<Room>> rooms;
    struct GameJob {
        std::shared_ptr<std::atomic<bool>> done;
        std::jthread thread;
    };
    std::vector<GameJob> games;

  public:
    bool room_exist(const std::string &name);
    bool add_room(const std::shared_ptr<Room> &room);
    void del_room(const std::string &name);
    std::shared_ptr<Room> get_object(const std::string &name);
    void launch(const std::shared_ptr<Room> &room);
    void shutdown();
};
} // namespace tetris::serveur
