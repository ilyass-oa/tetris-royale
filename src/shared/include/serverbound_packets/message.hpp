#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class Message : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::Message;
    std::string who;
    std::string texte;
    Message(std::string who, std::string texte) : who{std::move(who)}, texte{std::move(texte)} {}
    static std::shared_ptr<Message> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets