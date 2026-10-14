#include <iostream>
#include <memory>

#include <read.hpp>
#include <serverbound_packets/join_room.hpp>
#include <vector>
#include <write.hpp>

namespace tetris::serverbound_packets {
void JoinRoom::serialize(int socket) const {
    write(socket, ID);
    write(socket, room_name);
};
std::shared_ptr<JoinRoom> JoinRoom::recv(int socket) {
    std::string room_name;
    read(socket, room_name);
    return std::make_shared<JoinRoom>(room_name);
}
} // namespace tetris::serverbound_packets
