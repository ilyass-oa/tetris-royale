#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::clientbound_packets {
class GeneralResponse : public Packet {
  public:
    static constexpr ClientboundPacket ID = ClientboundPacket::GeneralResponse;
    int choice;
    bool success;
    std::string message;

    GeneralResponse(int choice, bool success, std::string message)
        : choice{choice}, success{success}, message{std::move(message)} {}
    static std::shared_ptr<GeneralResponse> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::clientbound_packets