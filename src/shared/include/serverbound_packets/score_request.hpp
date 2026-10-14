
#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class ScoreRequest : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::ScoreRequest;

    ScoreRequest() = default;
    static std::shared_ptr<ScoreRequest> recv(int socket);
    void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets