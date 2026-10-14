#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class SignIn : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::SignIn;
    std::string username;
    std::string password;
    SignIn(std::string username, std::string password)
        : username{std::move(username)}, password{std::move(password)} {}
    static std::shared_ptr<SignIn> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets