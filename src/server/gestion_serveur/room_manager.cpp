#include "room_manager.hpp"
#include "player.hpp"
#include "room.hpp"
#include <algorithm>
#include <iostream>
namespace tetris::serveur {
bool RoomsManager::room_exist(const std::string &name) {
    return bool(get_object(name));
}
bool RoomsManager::add_room(const std::shared_ptr<Room> &room) {
    auto name = room->get_name();
    std::lock_guard lock(mutex);
    return rooms.emplace(name, room).second;
}
void RoomsManager::del_room(const std::string &name) {
    std::lock_guard lock(mutex);
    rooms.erase(name);
}
std::shared_ptr<Room> RoomsManager::get_object(const std::string &name) {
    std::lock_guard lock(mutex);
    auto it = rooms.find(name);
    return it == rooms.end() ? nullptr : it->second;
}
void RoomsManager::launch(const std::shared_ptr<Room> &room) {
    std::lock_guard lock(mutex);
    std::erase_if(games, [](const GameJob &job) { return job.done->load(); });
    auto done = std::make_shared<std::atomic<bool>>(false);
    games.push_back({done, std::jthread([room, done] {
                         try {
                             room->run();
                         } catch (const std::exception &e) {
                             std::cerr << "Match ended: " << e.what() << '\n';
                         }
                         room->finish();
                         done->store(true);
                     })});
}
void RoomsManager::shutdown() {
    std::vector<GameJob> running;
    {
        std::lock_guard lock(mutex);
        for (auto &[_, room] : rooms)
            room->cancel();
        running.swap(games);
    }
    running.clear(); // Join outside the manager lock.
    std::lock_guard lock(mutex);
    rooms.clear();
}
} // namespace tetris::serveur
