#include <read.hpp>
#include <serverbound_packets/score_packet.hpp>
#include <write.hpp>

namespace tetris::serverbound_packets {

void ScorePacket::serialize(int socket) const {
    write(socket, ID);
    write(socket, score);
    write(socket, energy);
}

std::shared_ptr<ScorePacket> ScorePacket::recv(int socket) {
    int score, energy;
    read(socket, score);
    read(socket, energy);
    return std::make_shared<ScorePacket>(score, energy);
}

} // namespace tetris::serverbound_packets
