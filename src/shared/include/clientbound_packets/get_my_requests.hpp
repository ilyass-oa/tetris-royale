#pragma once
#include <memory>
#include <packet.hpp>
#include <string>
#include <vector>

namespace tetris::clientbound_packets {

class GetMyRequests : public Packet {
  public:
    std::vector<std::string> friendRequests;

    GetMyRequests(const std::vector<std::string> &friendRequests)
        : friendRequests(friendRequests) {}

    static std::shared_ptr<GetMyRequests> recv(int socket);
    virtual void serialize(int socket) const override;

    static constexpr ClientboundPacket ID = ClientboundPacket::GetMyRequests;
};

} // namespace tetris::clientbound_packets
