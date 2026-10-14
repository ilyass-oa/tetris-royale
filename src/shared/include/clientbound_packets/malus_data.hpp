#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::clientbound_packets {
class Malus : public Packet {
  public:
    static constexpr ClientboundPacket ID = ClientboundPacket::Malus;
    int malus;
    int player;
    Malus(int malus, int player) : malus(malus), player(player) {}
    static std::shared_ptr<Malus> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::clientbound_packets
