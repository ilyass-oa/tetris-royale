#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class Hello : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::Hello;
    bool create_account;
    Hello(bool create_account) : create_account{create_account} {}
    static std::shared_ptr<Hello> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets