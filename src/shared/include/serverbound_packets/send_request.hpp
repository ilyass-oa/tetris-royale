#pragma once

#include <memory>
#include <packet.hpp>
#include <string>

namespace tetris::serverbound_packets {
class SendRequest : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::SendRequest;
    std::string friendName;

    SendRequest(std::string friendName) : friendName{friendName} {}

    static std::shared_ptr<SendRequest> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets
