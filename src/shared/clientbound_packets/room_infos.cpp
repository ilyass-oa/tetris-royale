#include <iostream>
#include <memory>

#include <clientbound_packets/room_infos.hpp>
#include <read.hpp>
#include <write.hpp>

namespace tetris::clientbound_packets {
void RoomInfos::serialize(int socket) const {
    write(socket, ID);
    write(socket, numb_player);
    write(socket, room_name);
    write(socket, reponse);
    write(socket, mode);
    write(socket, owner);
};
std::shared_ptr<RoomInfos> RoomInfos::recv(int socket) {
    int numb_player;
    std::string room_name;
    bool reponse;
    read(socket, numb_player);
    read(socket, room_name);
    read(socket, reponse);
    GameMode mode;
    std::string owner;
    read(socket, mode);
    read(socket, owner);
    if (mode < GameMode::Menu || mode > GameMode::BR)
        throw transport::ProtocolError("invalid room mode");

    return std::make_shared<RoomInfos>(numb_player, room_name, reponse, mode, owner);
}
} // namespace tetris::clientbound_packets
