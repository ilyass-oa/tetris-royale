#pragma once

#include <memory>
#include <packet.hpp>
#include <string>

namespace tetris::clientbound_packets {
class GameEnd : public Packet {
  private:
    bool end;

  public:
    static constexpr ClientboundPacket ID = ClientboundPacket::GameEnd;
    GameEnd(bool end) : end{end} {}

    static std::shared_ptr<GameEnd> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::clientbound_packets