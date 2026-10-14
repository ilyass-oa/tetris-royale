#ifndef GRID_HPP
#define GRID_HPP

#include "Coordinate.hpp"
#include "constants.h"
#include <cstdlib>
#include <memory>
#include <shared_mutex>
#include <vector>

namespace tetris {

enum TileColor {
    CYAN,
    YELLOW,
    MAGENTA,
    GREEN,
    RED,
    BLUE,
    WHITE,
    GRAY,
    BLACK,
    EMPTY,
};

struct Tile {
    bool occupied;
    TileColor color;
};

class Grid {
  private:
    Tile tiles[BOARD_WIDTH][BOARD_HEIGHT];
    std::shared_mutex tiles_mutex;

  public:
    Grid();

    void newGrid(const std::vector<std::vector<int>> &gridData);
    void setOccupied(Coordinate c, bool occupied);
    void setTileColor(Coordinate c, TileColor color);
    void moveLinesDown(int start);
    void clearRow(int row);
    bool isOccupied(Coordinate c);
    TileColor getTileColor(Coordinate c);

    // Ajoute des lignes de malus en bas
    void addGrayLines(int numLines);
    void moveLinesUp();
};
} // namespace tetris

#endif // GRID_HPP
