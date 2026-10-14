#pragma once

#include <game/Malus.hpp>
#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class MalusPacket : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::MalusPacket;
    Malus malus;
    int player;
    MalusPacket(Malus malus, int player) : malus(malus), player(player) {}
    static std::shared_ptr<MalusPacket> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets