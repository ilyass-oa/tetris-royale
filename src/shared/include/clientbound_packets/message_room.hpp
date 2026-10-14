#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::clientbound_packets {
class MessageRoom : public Packet {
  public:
    static constexpr ClientboundPacket ID = ClientboundPacket::MessageRoom;
    std::string texte;
    MessageRoom(std::string texte) : texte{std::move(texte)} {}
    static std::shared_ptr<MessageRoom> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::clientbound_packets