#pragma once

#include <game/TetrisGame.hpp>
#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class SetStatus : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::SetStatus;
    GameMode status;
    SetStatus(GameMode status) : status(status) {}
    static std::shared_ptr<SetStatus> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets