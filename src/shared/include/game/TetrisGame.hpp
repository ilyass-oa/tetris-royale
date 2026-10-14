#ifndef TETRISGAME_HPP
#define TETRISGAME_HPP

#include "Grid.hpp"
#include "Piece.hpp"
#include "PieceFactory.hpp"
#include <memory>

namespace tetris {
enum class GameMode { Menu, Endless, Classique, Duel, BR };

class TetrisGame {
  private:
    std::unique_ptr<Piece> currentPiece;
    std::unique_ptr<PieceFactory> pieceFactory;
    bool isRunning = true;
    bool dropGrace = true;
    void applyKick();
    void placeCurrentPiece();
    void incrementScore(int lines);

    bool controlsInverted = false;
    bool controlsBlocked = false;
    bool oneByOneMode = false;
    int oneByOneCount = 0;
    double speedMultiplier = 1;
    int fallDelay = 250;
    std::shared_ptr<Grid> grid;

  public:
    TetrisGame();
    std::shared_ptr<Grid> getGrid();

    int score = 0, energy = 0;
    bool getRunning() const;
    int getScore() const;
    int getEnergyLevel() const;
    int totalLinesCleared = 0;
    bool isOccupied(Coordinate c) const;
    TileColor getColor(Coordinate c) const;
    TileColor getCurrentPieceColor() const;
    bool containsCurrentPiece(Coordinate c) const;
    void rotateCurrentPieceClockwise();
    void rotateCurrentPieceCounterclockwise();
    void rotateCurrentPiece180();
    void moveCurrentPiece(int dx, int dy);
    void checkForPlacement();
    int checkForLines();
    void setFallDelay(int delay);

    void applyGrayLines(int malusLines);
    void oneByOneBlocks();

    void update();
    void reset();
    void hardDrop();
    void end();
    PieceFactory *getPieceFactory() const;
    void clearTopLines(int numLines);
};
} // namespace tetris
#endif // TETRISGAME_HPP
