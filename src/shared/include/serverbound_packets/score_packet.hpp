#pragma once

#include <memory>
#include <packet.hpp>
#include <string>

namespace tetris::serverbound_packets {
class ScorePacket : public Packet {
  public:
    static std::shared_ptr<ScorePacket> recv(int socket);
    void serialize(int socket) const;
    ScorePacket(int score, int energy) : score(score), energy(energy) {}
    int score, energy;
    static constexpr ServerboundPacket ID = ServerboundPacket::ScorePacket;
};

} // namespace tetris::serverbound_packets
