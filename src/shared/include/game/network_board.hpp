#pragma once
#include "constants.h"
#include <algorithm>
#include <vector>

namespace tetris {
// The server reserves a complete red board as the eliminated-player marker.
// A normal red tetromino, or an empty board during blackout, is still alive.
inline bool isEliminatedNetworkBoard(const std::vector<std::vector<int>> &grid) {
    return grid.size() == BOARD_HEIGHT &&
           std::all_of(grid.begin(), grid.end(), [](const auto &row) {
               return row.size() == BOARD_WIDTH &&
                      std::all_of(row.begin(), row.end(), [](int cell) { return cell == 4; });
           });
}
} // namespace tetris
