#include <algorithm>
#include <game/Grid.hpp>
#include <mutex>
#include <stdexcept>
// Représente la grille du jeu (10x20), elle contient des tuiles qui peuvent être occupées ou non.
namespace tetris {
namespace {
void check(Coordinate c) {
    if (c.getX() < 0 || c.getX() >= BOARD_WIDTH || c.getY() < 0 || c.getY() >= BOARD_HEIGHT)
        throw std::out_of_range("Grid coordinate outside board");
}
} // namespace
Grid::Grid() {
    for (int i = 0; i < BOARD_WIDTH; i++) {
        for (int j = 0; j < BOARD_HEIGHT; j++) {
            tiles[i][j].occupied = false;
            tiles[i][j].color = TileColor::BLACK;
        }
    }
}

void Grid::newGrid(const std::vector<std::vector<int>> &gridData) {
    std::unique_lock<std::shared_mutex> lock(tiles_mutex);
    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        for (int x = 0; x < BOARD_WIDTH; ++x) {
            int colorCode = 8; // Valeur par défaut (vide)
            // Vérifier si la donnée existe dans gridData
            if (y < gridData.size() && x < gridData[y].size()) {
                colorCode = gridData[y][x];
            }
            // Si colorCode > 0, on considère la case occupée
            if (colorCode >= 0 && colorCode <= 7) {
                tiles[x][y].occupied = true;
                tiles[x][y].color =
                    static_cast<TileColor>(colorCode); // Convertir colorCode en TileColor
            } else {
                tiles[x][y].occupied = false;
                tiles[x][y].color = TileColor::BLACK; // Noir pour les cases vides
            }
        }
    }
}

void Grid::setOccupied(Coordinate coord, bool occupied) {
    check(coord);
    std::unique_lock<std::shared_mutex> lock(tiles_mutex);
    tiles[coord.getX()][coord.getY()].occupied = occupied;
}

bool Grid::isOccupied(Coordinate coord) {
    check(coord);
    std::shared_lock<std::shared_mutex> lock(tiles_mutex);
    return tiles[coord.getX()][coord.getY()].occupied;
}

void Grid::moveLinesDown(int start) {
    check(Coordinate(0, start));
    std::unique_lock<std::shared_mutex> lock(tiles_mutex);
    for (int i = start; i > 0; i--) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            tiles[j][i] = tiles[j][i - 1];
        }
    }
    for (int x = 0; x < BOARD_WIDTH; ++x)
        tiles[x][0] = {false, TileColor::BLACK};
}

void Grid::setTileColor(Coordinate coord, TileColor color) {
    check(coord);
    std::unique_lock<std::shared_mutex> lock(tiles_mutex);
    tiles[coord.getX()][coord.getY()].color = color;
}

TileColor Grid::getTileColor(Coordinate coord) {
    check(coord);
    std::shared_lock<std::shared_mutex> lock(tiles_mutex);
    return tiles[coord.getX()][coord.getY()].color;
}

void Grid::addGrayLines(int numLines) {
    numLines = std::clamp(numLines, 0, BOARD_HEIGHT);
    // Ici on veut décaler les lignes vers le haut pour faire de la place pour les lignes de malus
    // qu'on vas rejouter
    std::unique_lock<std::shared_mutex> lock(tiles_mutex);
    for (int y = 0; y < BOARD_HEIGHT - numLines; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            tiles[x][y] = tiles[x][y + numLines]; // Décale toutes les lignes vers le haut
        }
    }

    // Ajouter les nouvelles lignes de malus en bas
    for (int i = 0; i < numLines; i++) {
        int emptyCol = rand() % BOARD_WIDTH; // Choisir une colonne aléatoire à laisser vide

        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (x == emptyCol) {
                tiles[x][BOARD_HEIGHT - numLines + i].occupied = false;
                tiles[x][BOARD_HEIGHT - numLines + i].color =
                    TileColor::BLACK; // on met la case qu'on a choisi aléatoirement a vide
            } else {
                tiles[x][BOARD_HEIGHT - numLines + i].occupied =
                    true; // toutes les autres cases sont pleines
                tiles[x][BOARD_HEIGHT - numLines + i].color = TileColor::GRAY;
            }
        }
    }
}

void Grid::moveLinesUp() {
    std::unique_lock lock(tiles_mutex);
    for (int y = 0; y < BOARD_HEIGHT - 1; ++y)
        for (int x = 0; x < BOARD_WIDTH; ++x)
            tiles[x][y] = tiles[x][y + 1];
    for (int x = 0; x < BOARD_WIDTH; ++x)
        tiles[x][BOARD_HEIGHT - 1] = {false, TileColor::BLACK};
}

void Grid::clearRow(int row) {
    check(Coordinate(0, row));
    std::unique_lock<std::shared_mutex> lock(tiles_mutex);
    for (int x = 0; x < BOARD_WIDTH; ++x) {
        tiles[x][row].occupied = false;
        tiles[x][row].color = TileColor::EMPTY;
    }
}

} // namespace tetris