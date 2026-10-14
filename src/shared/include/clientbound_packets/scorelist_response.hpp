#pragma once
#include <memory>
#include <packet.hpp>
#include <string>
#include <vector>

namespace tetris::clientbound_packets {

class ScoreListResponse : public Packet {
  public:
    std::vector<std::string> topScoreList;

    ScoreListResponse(const std::vector<std::string> &topScoreList) : topScoreList(topScoreList) {}

    static std::shared_ptr<ScoreListResponse> recv(int socket);
    virtual void serialize(int socket) const override;

    static constexpr ClientboundPacket ID = ClientboundPacket::ScoreListResponse;
};

} // namespace tetris::clientbound_packets