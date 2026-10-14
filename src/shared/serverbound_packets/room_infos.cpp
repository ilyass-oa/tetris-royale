#include <iostream>
#include <memory>

#include <read.hpp>
#include <serverbound_packets/room_infos.hpp>
#include <write.hpp>

namespace tetris::serverbound_packets {
void RoomInfos::serialize(int socket) const {
    write(socket, ID);
    write(socket, room_name);
};
std::shared_ptr<RoomInfos> RoomInfos::recv(int socket) {
    std::string room_name;
    read(socket, room_name);
    return std::make_shared<RoomInfos>(room_name);
}
} // namespace tetris::serverbound_packets
