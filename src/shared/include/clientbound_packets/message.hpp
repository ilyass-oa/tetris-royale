#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::clientbound_packets {
class Message : public Packet {
  public:
    static constexpr ClientboundPacket ID = ClientboundPacket::Message;
    std::string texte;
    Message(std::string texte) : texte{std::move(texte)} {}
    static std::shared_ptr<Message> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::clientbound_packets