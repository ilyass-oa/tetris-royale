#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class DelRoom : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::DelRoom;
    std::string pseudo;
    DelRoom(std::string pseudo) : pseudo(pseudo) {}
    static std::shared_ptr<DelRoom> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets
