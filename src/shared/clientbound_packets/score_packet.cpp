#include <iostream>
#include <memory>

#include <clientbound_packets/score_packet.hpp>
#include <read.hpp>
#include <write.hpp>

namespace tetris::clientbound_packets {
void ScorePacket::serialize(int socket) const {
    write(socket, ClientboundPacket::ScorePacket);
    write(socket, score);
    write(socket, energy);
};
std::shared_ptr<ScorePacket> ScorePacket::recv(int socket) {
    int score, energy;
    read(socket, score);
    read(socket, energy);
    return std::make_shared<ScorePacket>(score, energy);
}
} // namespace tetris::clientbound_packets