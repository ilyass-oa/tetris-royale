#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class RoomInfos : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::RoomInfos;
    std::string room_name;
    RoomInfos(std::string room_name) : room_name(room_name) {}
    static std::shared_ptr<RoomInfos> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets
