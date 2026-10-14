
#include <iostream>
#include <memory>

#include <read.hpp>
#include <serverbound_packets/delete_room.hpp>
#include <vector>
#include <write.hpp>

namespace tetris::serverbound_packets {
void DelRoom::serialize(int socket) const {
    write(socket, ID);
    write(socket, pseudo);
};
std::shared_ptr<DelRoom> DelRoom::recv(int socket) {
    std::string pseudo;
    read(socket, pseudo);
    return std::make_shared<DelRoom>(pseudo);
}
} // namespace tetris::serverbound_packets
