#pragma once
#include <memory>
#include <packet.hpp>
#include <string>
#include <vector>

namespace tetris::clientbound_packets {

class FriendsListResponse : public Packet {
  public:
    std::vector<std::string> friendsList;

    FriendsListResponse(const std::vector<std::string> &friendsList) : friendsList(friendsList) {}

    static std::shared_ptr<FriendsListResponse> recv(int socket);
    virtual void serialize(int socket) const override;

    static constexpr ClientboundPacket ID = ClientboundPacket::FriendsListResponse;
};

} // namespace tetris::clientbound_packets