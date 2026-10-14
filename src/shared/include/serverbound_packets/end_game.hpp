#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class GameEnd : public Packet {
    bool end;

  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::GameEnd;
    GameEnd(bool end) : end(end) {}
    static std::shared_ptr<GameEnd> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets