#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::clientbound_packets {
class GameStart : public Packet {
    int rien = 0;

  public:
    static constexpr ClientboundPacket ID = ClientboundPacket::GameStart;
    GameStart() {}
    static std::shared_ptr<GameStart> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::clientbound_packets