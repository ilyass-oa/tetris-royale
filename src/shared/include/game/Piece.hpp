#ifndef PIECE_HPP
#define PIECE_HPP

#include "Coordinate.hpp"
#include "Grid.hpp"
namespace tetris {

enum PieceType { I, O, T, S, Z, J, L, CUSTOM };

class Piece {
  private:
    PieceType type;
    Coordinate position;
    int rotation;
    int prevRotation; // For wall kicks
    const Coordinate *getRotationCoords() const;

  public:
    Piece(PieceType type, Coordinate position);
    bool occupies(Coordinate coord) const;
    void move(int dx, int dy);
    void rotateClockwise();
    void rotateCounterclockwise();
    void rotate180();
    void revertRotation();
    Coordinate getPosition() const;
    PieceType getType() const;
    int getRotation() const;
    int getPrevRotation() const;
    bool collidesWith(Grid &grid) const;
    void placeOn(Grid &grid) const;
    bool isOutOfBounds() const;
    TileColor getColor() const;
    std::vector<std::vector<int>> getShape() const;
};
} // namespace tetris
#endif // PIECE_HPP
