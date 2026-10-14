#include <iostream>
#include <memory>

#include <clientbound_packets/malus_data.hpp>
#include <read.hpp>
#include <write.hpp>

namespace tetris::clientbound_packets {
void Malus::serialize(int socket) const {
    write(socket, ID);
    write(socket, player);
    write(socket, malus);
};
std::shared_ptr<Malus> Malus::recv(int socket) {
    int player;
    int malus;
    read(socket, player);
    read(socket, malus);
    return std::make_shared<Malus>(malus, player);
}
} // namespace tetris::clientbound_packets
