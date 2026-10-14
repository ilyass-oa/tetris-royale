#pragma once

#include <game/Piece.hpp>
#include <memory>
#include <packet.hpp>

namespace tetris::clientbound_packets {
class NextTile : public Packet {
  public:
    static constexpr ClientboundPacket ID = ClientboundPacket::NextTile;
    PieceType next_piece;
    NextTile(PieceType next_piece) : next_piece{next_piece} {}
    static std::shared_ptr<NextTile> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::clientbound_packets