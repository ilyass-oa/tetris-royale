#include <iostream>
#include <memory>

#include <read.hpp>
#include <serverbound_packets/create_room.hpp>
#include <vector>
#include <write.hpp>

namespace tetris::serverbound_packets {
void CreateRoom::serialize(int socket) const {
    write(socket, ID);
    write(socket, limit);
    write(socket, room_name);
    write(socket, game_mode);
};
std::shared_ptr<CreateRoom> CreateRoom::recv(int socket) {
    int limit;
    std::string room_name;
    GameMode game_mode;

    read(socket, limit);
    read(socket, room_name);
    read(socket, game_mode);
    if (game_mode < GameMode::Menu || game_mode > GameMode::BR) {
        throw transport::ProtocolError("invalid game mode");
    }
    return std::make_shared<CreateRoom>(limit, room_name, game_mode);
}
} // namespace tetris::serverbound_packets
