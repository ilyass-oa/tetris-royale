#pragma once
#include <memory>
#include <packet.hpp>
#include <string>

namespace tetris::serverbound_packets {

class ScoreListRequest : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::ScoreListRequest;

    ScoreListRequest() = default;

    void serialize(int socket) const override;

    static std::shared_ptr<ScoreListRequest> recv(int socket);
};

} // namespace tetris::serverbound_packets
