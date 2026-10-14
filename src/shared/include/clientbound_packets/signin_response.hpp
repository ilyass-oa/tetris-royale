#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::clientbound_packets {
class SignInResponse : public Packet {
  public:
    static constexpr ClientboundPacket ID = ClientboundPacket::SignInResponse;
    bool success;
    SignInResponse(bool success) : success{success} {}
    static std::shared_ptr<SignInResponse> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::clientbound_packets