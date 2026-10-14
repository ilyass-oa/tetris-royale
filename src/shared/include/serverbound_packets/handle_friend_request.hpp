#pragma once

#include <memory>
#include <packet.hpp>
#include <string>

namespace tetris::serverbound_packets {
class HandleFriendRequest : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::HandleFriendRequest;
    std::string friendName;
    bool accepted;

    HandleFriendRequest(std::string friendName, bool accepted)
        : friendName{friendName}, accepted{accepted} {}

    static std::shared_ptr<HandleFriendRequest> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets
