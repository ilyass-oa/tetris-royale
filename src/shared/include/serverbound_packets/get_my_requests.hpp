#pragma once
#include <memory>
#include <packet.hpp>
#include <string>

namespace tetris::serverbound_packets {

class GetMyRequests : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::GetMyRequests;

    GetMyRequests() = default;

    void serialize(int socket) const override;

    static std::shared_ptr<GetMyRequests> recv(int socket);
};

} // namespace tetris::serverbound_packets
