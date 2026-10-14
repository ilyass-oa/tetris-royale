#ifndef PIECEFACTORY_HPP
#define PIECEFACTORY_HPP

#include "Piece.hpp"
#include <random>

namespace tetris {
class PieceFactory {
  private:
    std::default_random_engine rng;
    std::vector<PieceType> allPieces;
    std::vector<PieceType> bag;

    void refillBag();
    PieceType drawPiece();

  public:
    PieceFactory();
    std::unique_ptr<Piece> createPiece();
    std::unique_ptr<Piece> createOneByOnePiece();

    PieceType peekNextPiece();
};
} // namespace tetris

#endif // PIECEFACTORY_HPP
