#include <iostream>
#include <memory>

#include <clientbound_packets/next_tile.hpp>
#include <game/Piece.hpp>
#include <read.hpp>
#include <write.hpp>

namespace tetris::clientbound_packets {
void NextTile::serialize(int socket) const {
    write(socket, ClientboundPacket::NextTile);
    write(socket, next_piece);
};
std::shared_ptr<NextTile> NextTile::recv(int socket) {
    PieceType next_piece;
    read(socket, next_piece);
    if (next_piece < I || next_piece > CUSTOM) {
        throw transport::ProtocolError("invalid piece type");
    }
    return std::make_shared<NextTile>(next_piece);
}
} // namespace tetris::clientbound_packets
