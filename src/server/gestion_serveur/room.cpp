#include "room.hpp"
namespace tetris::serveur {
Room::Room(int capacity, std::string owner, std::string name,
           std::shared_ptr<PlayersManager> players, std::shared_ptr<RoomsManager>, GameMode mode)
    : owner(std::move(owner)), room_name(std::move(name)), capacity(capacity), mode(mode),
      players(std::move(players)) {
    members.insert(this->owner);
}
bool Room::add_player(const std::string &name) {
    std::lock_guard lock(mutex);
    if (playing || members.size() >= static_cast<std::size_t>(capacity))
        return false;
    return members.insert(name).second;
}
void Room::del_player(const std::string &name) {
    std::lock_guard lock(mutex);
    members.erase(name);
    if (!playing && name == owner)
        owner = members.empty() ? "" : *members.begin();
}
std::string Room::get_name() const {
    std::lock_guard lock(mutex);
    return room_name;
}
std::string Room::get_owner() const {
    std::lock_guard lock(mutex);
    return owner;
}
bool Room::set_capacity(int requested) {
    std::lock_guard lock(mutex);
    if (playing || requested < 1 || requested > capacity ||
        (mode != GameMode::Endless && requested < 2))
        return false;
    capacity = requested;
    return true;
}
int Room::get_players_number() const {
    std::lock_guard lock(mutex);
    return members.size();
}
bool Room::is_owner(const std::string &name) const {
    std::lock_guard lock(mutex);
    return owner == name;
}
bool Room::try_start(const std::string &name, GameMode requested) {
    std::lock_guard lock(mutex);
    auto minimum = mode == GameMode::Endless ? 1u : 2u;
    if (playing || cancelled.load() || name != owner || requested != mode ||
        members.size() < minimum)
        return false;
    playing = true;
    return true;
}
void Room::finish() {
    std::lock_guard lock(mutex);
    playing = false;
    if (!members.contains(owner))
        owner = members.empty() ? "" : *members.begin();
}
bool Room::is_playing() const {
    std::lock_guard lock(mutex);
    return playing;
}
std::unordered_set<std::string> Room::get_deep_cpy_list(const std::string &except) const {
    std::lock_guard lock(mutex);
    auto result = members;
    result.erase(except);
    return result;
}
} // namespace tetris::serveur
