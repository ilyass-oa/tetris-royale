#pragma once

#include <game/TetrisGame.hpp>
#include <memory>
#include <packet.hpp>
#include <string>

namespace tetris::clientbound_packets {
class RoomInfos : public Packet {
  public:
    static constexpr ClientboundPacket ID = ClientboundPacket::RoomInfos;
    int numb_player;
    std::string room_name;
    bool reponse;
    GameMode mode;
    std::string owner;
    RoomInfos(int numb_player, std::string room_name, bool reponse, GameMode mode = GameMode::Menu,
              std::string owner = "")
        : numb_player(numb_player), room_name{std::move(room_name)}, reponse(reponse), mode(mode),
          owner(std::move(owner)) {}
    static std::shared_ptr<RoomInfos> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::clientbound_packets
