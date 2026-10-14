#pragma once

#include <game/TetrisGame.hpp>
#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class CreateRoom : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::CreateRoom;
    int limit;
    std::string room_name;
    GameMode game_mode;
    CreateRoom(int limit, std::string room_name, GameMode game_mode)
        : limit(limit), room_name{std::move(room_name)}, game_mode(game_mode) {}
    static std::shared_ptr<CreateRoom> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets
