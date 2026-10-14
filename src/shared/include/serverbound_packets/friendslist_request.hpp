#pragma once
#include <memory>
#include <packet.hpp>
#include <string>

namespace tetris::serverbound_packets {

class FriendsListRequest : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::FriendsListRequest;

    FriendsListRequest() = default;

    void serialize(int socket) const override;

    static std::shared_ptr<FriendsListRequest> recv(int socket);
};

} // namespace tetris::serverbound_packets
