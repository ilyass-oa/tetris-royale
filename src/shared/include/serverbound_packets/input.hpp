#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class Input : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::Input;
    int input;
    Input(int input) : input(input) {}
    static std::shared_ptr<Input> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets