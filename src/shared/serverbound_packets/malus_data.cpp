#include <iostream>
#include <memory>

#include <read.hpp>
#include <serverbound_packets/malus_data.hpp>
#include <write.hpp>

namespace tetris::serverbound_packets {
void MalusPacket::serialize(int socket) const {
    write(socket, ID);
    write(socket, player);
    write(socket, malus);
};
std::shared_ptr<MalusPacket> MalusPacket::recv(int socket) {
    int player;
    Malus malus;
    read(socket, player);
    read(socket, malus);
    return std::make_shared<MalusPacket>(malus, player);
}
} // namespace tetris::serverbound_packets
