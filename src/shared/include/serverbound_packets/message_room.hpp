#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class MessageRoom : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::MessageRoom;
    std::string room;
    std::string texte;
    MessageRoom(std::string room, std::string texte)
        : room{std::move(room)}, texte{std::move(texte)} {}
    static std::shared_ptr<MessageRoom> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets