#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::clientbound_packets {
class ScoreResponse : public Packet {
  public:
    static constexpr ClientboundPacket ID = ClientboundPacket::ScoreResponse;
    int score;
    ScoreResponse(int score) : score(score) {}
    static std::shared_ptr<ScoreResponse> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::clientbound_packets