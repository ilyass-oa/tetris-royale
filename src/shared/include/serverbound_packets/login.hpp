#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class Login : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::Login;
    std::string username;
    std::string password;
    Login(std::string username, std::string password)
        : username{std::move(username)}, password{std::move(password)} {}
    static std::shared_ptr<Login> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets