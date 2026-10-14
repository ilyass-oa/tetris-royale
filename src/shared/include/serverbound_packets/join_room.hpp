#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class JoinRoom : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::JoinRoom;
    std::string room_name;
    JoinRoom(std::string room_name) : room_name(room_name) {}
    static std::shared_ptr<JoinRoom> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets
