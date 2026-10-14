#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::clientbound_packets {
class LoginResponse : public Packet {
  public:
    static constexpr ClientboundPacket ID = ClientboundPacket::LoginResponse;
    bool success;
    LoginResponse(bool success) : success{success} {}
    static std::shared_ptr<LoginResponse> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::clientbound_packets