#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::clientbound_packets {
class ScorePacket : public Packet {
  public:
    static constexpr ClientboundPacket ID = ClientboundPacket::ScorePacket;
    int score, energy;
    ScorePacket(int score, int energy = 0) : score{score}, energy(energy) {}
    static std::shared_ptr<ScorePacket> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::clientbound_packets