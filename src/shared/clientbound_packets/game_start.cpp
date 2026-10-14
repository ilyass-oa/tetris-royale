#include <iostream>
#include <memory>

#include <clientbound_packets/game_start.hpp>
#include <read.hpp>
#include <write.hpp>

namespace tetris::clientbound_packets {
void GameStart::serialize(int socket) const {
    write(socket, ClientboundPacket::GameStart);
    write(socket, rien);
};
std::shared_ptr<GameStart> GameStart::recv(int socket) {
    int rien;
    read(socket, rien);
    return std::make_shared<GameStart>();
}
} // namespace tetris::clientbound_packets
