#pragma once

#include <game/TetrisGame.hpp>
#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class SetReady : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::SetReady;
    GameMode game_mode;
    SetReady(GameMode game_mode) : game_mode(game_mode) {}
    static std::shared_ptr<SetReady> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets