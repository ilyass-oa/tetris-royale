#include "player.hpp"
#include "../game/br.hpp"
#include "../game/classique.hpp"
#include "../game/duel.hpp"
#include "../game/endless.hpp"
#include "room.hpp"
#include <algorithm>
#include <iostream>
#include <packet_read.hpp>
#include <unistd.h>

namespace tetris::serveur {
Player::Player(int fd, sockaddr_in, std::shared_ptr<PlayersManager> players,
               std::shared_ptr<RoomsManager> rooms, std::shared_ptr<DatabaseManager> data)
    : socket_fd(fd), players(std::move(players)), rooms(std::move(rooms)), data(std::move(data)) {}
Player::~Player() {
    exit();
    transport::forget_socket(socket_fd);
    ::close(socket_fd);
}
std::string Player::get_name() const {
    std::lock_guard lock(state_mutex);
    return name;
}
std::string Player::get_current_room() const {
    std::lock_guard lock(state_mutex);
    return current_room;
}
bool Player::is_playing_in(const std::string &room) const {
    return is_connected() && current_mode.load() != GameMode::Menu && get_current_room() == room;
}
void Player::input_purge() {
    std::lock_guard lock(input_mutex);
    inputs.clear();
    attacks.clear();
}
void Player::input_push(int input) {
    std::lock_guard lock(input_mutex);
    if (inputs.size() < 128)
        inputs.push_back(input);
}
int Player::input_pop() {
    std::lock_guard lock(input_mutex);
    if (inputs.empty())
        return '+';
    int next = inputs.front();
    inputs.pop_front();
    return next;
}
std::tuple<int, Malus> Player::malus_to_other_pop() {
    std::lock_guard lock(input_mutex);
    if (attacks.empty())
        return {-1, Malus::NO};
    auto next = attacks.front();
    attacks.pop_front();
    return next;
}
void Player::publish_room() {
    auto room = rooms->get_object(get_current_room());
    if (!room)
        return;
    auto members = room->get_deep_cpy_list();
    for (const auto &member : members)
        if (auto player = players->get_object(member)) {
            try {
                player->send_packet<clientbound_packets::RoomInfos>(
                    room->get_players_number(), room->get_name(), true, room->get_mode(),
                    room->get_owner());
            } catch (const std::exception &) {
            }
        }
}
void Player::leave_room() {
    std::string previous;
    {
        std::lock_guard lock(state_mutex);
        previous = std::move(current_room);
        current_room.clear();
    }
    current_mode.store(GameMode::Menu);
    if (auto room = rooms->get_object(previous)) {
        room->del_player(get_name());
        if (room->get_players_number() == 0) {
            room->cancel();
            rooms->del_room(previous);
        } else
            for (const auto &member : room->get_deep_cpy_list())
                if (auto player = players->get_object(member)) {
                    try {
                        player->send_packet<clientbound_packets::RoomInfos>(
                            room->get_players_number(), previous, true, room->get_mode(),
                            room->get_owner());
                    } catch (const std::exception &) {
                    }
                }
    }
    input_purge();
}
void Player::exit() {
    quit.store(true);
    ::shutdown(socket_fd, SHUT_RDWR);
    if (cleaned.exchange(true))
        return;
    leave_room();
    players->del_player(get_name(), this);
}
bool Player::authenticate() {
    auto hello = read_specific_serverbound_packet<serverbound_packets::Hello>(socket_fd);
    bool success = false;
    std::string username, password;
    if (hello->create_account) {
        auto credentials = read_specific_serverbound_packet<serverbound_packets::SignIn>(socket_fd);
        username = credentials->username;
        password = credentials->password;
        success = data->addUser(username, password);
    } else {
        auto credentials = read_specific_serverbound_packet<serverbound_packets::Login>(socket_fd);
        username = credentials->username;
        password = credentials->password;
        success = data->verifyUser(username, password);
    }
    if (success) {
        {
            std::lock_guard lock(state_mutex);
            name = username;
        }
        success = players->add_player(shared_from_this());
    }
    if (hello->create_account)
        send_packet<clientbound_packets::SignInResponse>(success);
    else
        send_packet<clientbound_packets::LoginResponse>(success);
    return success;
}
void Player::player_thread() {
    try {
        if (authenticate())
            while (!quit.load())
                handle(read_serverbound_packet(socket_fd));
    } catch (const std::exception &e) {
        std::cerr << "Session closed: " << e.what() << '\n';
    }
    exit();
    done.store(true);
}
void Player::create_room(int limit, const std::string &roomName, GameMode mode) {
    if (!get_current_room().empty() || roomName.empty() || roomName.size() > 48 ||
        std::any_of(roomName.begin(), roomName.end(),
                    [](unsigned char c) { return c < 32 || c == 127; }) ||
        mode < GameMode::Endless || mode > GameMode::BR || limit < 1 || limit > 9) {
        send_packet<clientbound_packets::RoomInfos>(0, "", false);
        return;
    }
    std::shared_ptr<Room> room;
    switch (mode) {
    case GameMode::Endless:
        room = std::make_shared<Endless>(get_name(), roomName, players, rooms);
        break;
    case GameMode::Classique:
        room = std::make_shared<Classique>(get_name(), roomName, players, rooms);
        break;
    case GameMode::Duel:
        room = std::make_shared<Duel>(get_name(), roomName, players, rooms);
        break;
    case GameMode::BR:
        room = std::make_shared<BR>(get_name(), roomName, players, rooms);
        break;
    default:
        break;
    }
    if (!room || !room->set_capacity(limit) || !rooms->add_room(room)) {
        send_packet<clientbound_packets::RoomInfos>(0, "", false);
        return;
    }
    {
        std::lock_guard lock(state_mutex);
        current_room = roomName;
    }
    publish_room();
}
void Player::join_room(const std::string &roomName) {
    auto room = rooms->get_object(roomName);
    if (!get_current_room().empty() || !room || !room->add_player(get_name())) {
        send_packet<clientbound_packets::RoomInfos>(0, "", false);
        return;
    }
    {
        std::lock_guard lock(state_mutex);
        current_room = roomName;
    }
    publish_room();
}
void Player::start_game(GameMode mode) {
    auto room = rooms->get_object(get_current_room());
    if (!room) {
        send_packet<clientbound_packets::RoomInfos>(0, "", false);
        return;
    }
    if (!room->try_start(get_name(), mode)) {
        send_packet<clientbound_packets::RoomInfos>(room->get_players_number(), room->get_name(),
                                                    true, room->get_mode(), room->get_owner());
        return;
    }
    for (const auto &member : room->get_deep_cpy_list())
        if (auto player = players->get_object(member)) {
            player->input_purge();
            player->change_status(mode);
            try {
                player->send_packet<clientbound_packets::GameStart>();
            } catch (const std::exception &) {
            }
        }
    rooms->launch(room);
}
void Player::handle(const std::shared_ptr<Packet> &packet) {
    if (std::dynamic_pointer_cast<serverbound_packets::Hello>(packet)) {
        exit();
        return;
    }
    if (auto p = std::dynamic_pointer_cast<serverbound_packets::SetStatus>(packet)) {
        if (p->status == GameMode::Menu)
            leave_room(); // Modes are assigned only by server-side match start.
        return;
    }
    if (std::dynamic_pointer_cast<serverbound_packets::GameEnd>(packet) ||
        std::dynamic_pointer_cast<serverbound_packets::DelRoom>(packet)) {
        leave_room();
        return;
    }
    if (auto p = std::dynamic_pointer_cast<serverbound_packets::Input>(packet)) {
        if (current_mode.load() != GameMode::Menu)
            input_push(p->input);
        return;
    }
    if (auto p = std::dynamic_pointer_cast<serverbound_packets::MalusPacket>(packet)) {
        if (current_mode.load() == GameMode::BR && p->player >= 0 && p->player <= 9 &&
            p->malus > Malus::NO && p->malus <= Malus::STRIKE) {
            std::lock_guard lock(input_mutex);
            if (attacks.size() < 16)
                attacks.emplace_back(p->player, p->malus);
        }
        return;
    }
    if (current_mode.load() != GameMode::Menu)
        return;
    if (auto p = std::dynamic_pointer_cast<serverbound_packets::CreateRoom>(packet)) {
        create_room(p->limit, p->room_name, p->game_mode);
        return;
    }
    if (auto p = std::dynamic_pointer_cast<serverbound_packets::JoinRoom>(packet)) {
        join_room(p->room_name);
        return;
    }
    if (auto p = std::dynamic_pointer_cast<serverbound_packets::SetReady>(packet)) {
        start_game(p->game_mode);
        return;
    }
    if (std::dynamic_pointer_cast<serverbound_packets::ScorePacket>(packet))
        return; // Match loop alone saves scores.
    if (std::dynamic_pointer_cast<serverbound_packets::ScoreRequest>(packet)) {
        send_packet<clientbound_packets::ScoreResponse>(data->getUserScore(get_name()));
        return;
    }
    if (auto p = std::dynamic_pointer_cast<serverbound_packets::SendRequest>(packet)) {
        data->sendFriendRequest(get_name(), p->friendName);
        return;
    }
    if (std::dynamic_pointer_cast<serverbound_packets::GetMyRequests>(packet)) {
        send_packet<clientbound_packets::GetMyRequests>(data->getFriendRequests(get_name()));
        return;
    }
    if (std::dynamic_pointer_cast<serverbound_packets::FriendsListRequest>(packet)) {
        send_packet<clientbound_packets::FriendsListResponse>(data->getFriendsList(get_name()));
        return;
    }
    if (std::dynamic_pointer_cast<serverbound_packets::ScoreListRequest>(packet)) {
        send_packet<clientbound_packets::ScoreListResponse>(data->getTopScores());
        return;
    }
    if (auto p = std::dynamic_pointer_cast<serverbound_packets::HandleFriendRequest>(packet)) {
        if (p->accepted)
            data->acceptFriendRequest(get_name(), p->friendName);
        else
            data->removeFriend(get_name(), p->friendName);
        return;
    }
    if (auto p = std::dynamic_pointer_cast<serverbound_packets::Message>(packet)) {
        if (auto target = players->get_object(p->who))
            try {
                target->send_packet<clientbound_packets::Message>("(" + get_name() + ") " +
                                                                  p->texte);
            } catch (const std::exception &) {
            }
        return;
    }
    if (auto p = std::dynamic_pointer_cast<serverbound_packets::MessageRoom>(packet)) {
        auto room = rooms->get_object(get_current_room());
        if (!room || p->room != room->get_name())
            return;
        for (const auto &member : room->get_deep_cpy_list())
            if (auto target = players->get_object(member))
                try {
                    target->send_packet<clientbound_packets::Message>("(" + get_name() + ") " +
                                                                      p->texte);
                } catch (const std::exception &) {
                }
    }
}
} // namespace tetris::serveur
