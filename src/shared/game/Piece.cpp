#include <game/Piece.hpp>
#include <stdexcept>

// Pour représenter une pièce/tetrimino dans le jeu, avec sa forme et sa rotation.

namespace tetris {
// Basé sur les rotations de Guideline
// https://tetris.wiki/Super_Rotation_System#How_Guideline_SRS_Really_Works
static const Coordinate I_PIECE_ROTATIONS[4][4] = {
    {Coordinate(1, 3), Coordinate(2, 3), Coordinate(3, 3), Coordinate(4, 3)},
    {Coordinate(2, 1), Coordinate(2, 2), Coordinate(2, 3), Coordinate(2, 4)},
    {Coordinate(0, 3), Coordinate(1, 3), Coordinate(2, 3), Coordinate(3, 3)},
    {Coordinate(2, 0), Coordinate(2, 1), Coordinate(2, 2), Coordinate(2, 3)}};

static const Coordinate J_PIECE_ROTATIONS[4][4] = {
    {Coordinate(0, 0), Coordinate(0, 1), Coordinate(1, 1), Coordinate(2, 1)},
    {Coordinate(1, 0), Coordinate(2, 0), Coordinate(1, 1), Coordinate(1, 2)},
    {Coordinate(0, 1), Coordinate(1, 1), Coordinate(2, 1), Coordinate(2, 2)},
    {Coordinate(1, 0), Coordinate(1, 1), Coordinate(1, 2), Coordinate(0, 2)}};

static const Coordinate L_PIECE_ROTATIONS[4][4] = {
    {Coordinate(2, 0), Coordinate(0, 1), Coordinate(1, 1), Coordinate(2, 1)},
    {Coordinate(1, 0), Coordinate(1, 1), Coordinate(1, 2), Coordinate(2, 2)},
    {Coordinate(0, 1), Coordinate(1, 1), Coordinate(2, 1), Coordinate(0, 2)},
    {Coordinate(0, 0), Coordinate(1, 0), Coordinate(1, 1), Coordinate(1, 2)}};

static const Coordinate O_PIECE_ROTATIONS[4][4] = {
    {Coordinate(1, 0), Coordinate(2, 0), Coordinate(1, 1), Coordinate(2, 1)},
    {Coordinate(1, 1), Coordinate(2, 1), Coordinate(1, 2), Coordinate(2, 2)},
    {Coordinate(0, 1), Coordinate(1, 1), Coordinate(0, 2), Coordinate(1, 2)},
    {Coordinate(0, 0), Coordinate(1, 0), Coordinate(0, 1), Coordinate(1, 1)}};

static const Coordinate S_PIECE_ROTATIONS[4][4] = {
    {Coordinate(1, 0), Coordinate(2, 0), Coordinate(0, 1), Coordinate(1, 1)},
    {Coordinate(1, 0), Coordinate(1, 1), Coordinate(2, 1), Coordinate(2, 2)},
    {Coordinate(1, 1), Coordinate(2, 1), Coordinate(0, 2), Coordinate(1, 2)},
    {Coordinate(0, 0), Coordinate(0, 1), Coordinate(1, 1), Coordinate(1, 2)}};

static const Coordinate T_PIECE_ROTATIONS[4][4] = {
    {Coordinate(1, 0), Coordinate(0, 1), Coordinate(1, 1), Coordinate(2, 1)},
    {Coordinate(1, 0), Coordinate(1, 1), Coordinate(2, 1), Coordinate(1, 2)},
    {Coordinate(0, 1), Coordinate(1, 1), Coordinate(2, 1), Coordinate(1, 2)},
    {Coordinate(1, 0), Coordinate(0, 1), Coordinate(1, 1), Coordinate(1, 2)}};

static const Coordinate Z_PIECE_ROTATIONS[4][4] = {
    {Coordinate(0, 0), Coordinate(1, 0), Coordinate(1, 1), Coordinate(2, 1)},
    {Coordinate(2, 0), Coordinate(1, 1), Coordinate(2, 1), Coordinate(1, 2)},
    {Coordinate(0, 1), Coordinate(1, 1), Coordinate(1, 2), Coordinate(2, 2)},
    {Coordinate(1, 0), Coordinate(0, 1), Coordinate(1, 1), Coordinate(0, 2)}};

static const Coordinate CUSTOM_PIECE_ROTATIONS[4][4] = {
    {Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, 0)},
    {Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, 0)},
    {Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, 0)},
    {Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, 0)}};

Piece::Piece(PieceType type, Coordinate position)
    : type(type), position(position), rotation(0), prevRotation(0) {
    if (type < PieceType::I || type > PieceType::CUSTOM)
        throw std::invalid_argument("Invalid piece type");
}

Coordinate Piece::getPosition() const {
    return position;
}

PieceType Piece::getType() const {
    return type;
}

int Piece::getRotation() const {
    return rotation;
}

int Piece::getPrevRotation() const {
    return prevRotation;
}

std::vector<std::vector<int>> Piece::getShape() const {
    switch (type) {
    case PieceType::I:
        return {{1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
    case PieceType::J:
        return {{1, 0, 0}, {1, 1, 1}, {0, 0, 0}};
    case PieceType::L:
        return {{0, 0, 1}, {1, 1, 1}, {0, 0, 0}};
    case PieceType::O:
        return {{1, 1}, {1, 1}};
    case PieceType::S:
        return {{0, 1, 1}, {1, 1, 0}, {0, 0, 0}};
    case PieceType::T:
        return {{0, 1, 0}, {1, 1, 1}, {0, 0, 0}};
    case PieceType::Z:
        return {{1, 1, 0}, {0, 1, 1}, {0, 0, 0}};
    case PieceType::CUSTOM:
        return {{1}};
    default:
        return {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
    }
}

const Coordinate *Piece::getRotationCoords() const {
    switch (getType()) {
    case PieceType::I:
        return I_PIECE_ROTATIONS[getRotation()];
    case PieceType::J:
        return J_PIECE_ROTATIONS[getRotation()];
    case PieceType::L:
        return L_PIECE_ROTATIONS[getRotation()];
    case PieceType::O:
        return O_PIECE_ROTATIONS[getRotation()];
    case PieceType::S:
        return S_PIECE_ROTATIONS[getRotation()];
    case PieceType::T:
        return T_PIECE_ROTATIONS[getRotation()];
    case PieceType::Z:
        return Z_PIECE_ROTATIONS[getRotation()];
    case PieceType::CUSTOM:
        return CUSTOM_PIECE_ROTATIONS[getRotation()];
    default:
        return nullptr;
    }
}

bool Piece::occupies(Coordinate coord) const {
    const Coordinate *coords = getRotationCoords();
    for (int i = 0; i < 4; i++) {
        if (coords[i] + getPosition() == coord) {
            return true;
        }
    }
    return false;
}

void Piece::move(int dx, int dy) {
    position.setX(position.getX() + dx);
    position.setY(position.getY() + dy);
}

void Piece::rotateClockwise() {
    prevRotation = rotation;
    rotation = (rotation + 1) % 4;
}

void Piece::rotateCounterclockwise() {
    prevRotation = rotation;
    rotation = (rotation + 3) % 4;
}

void Piece::rotate180() {
    prevRotation = rotation;
    rotation = (rotation + 2) % 4;
}

void Piece::revertRotation() {
    rotation = prevRotation;
}

bool Piece::collidesWith(Grid &grid) const {
    const Coordinate *coords = getRotationCoords();
    for (int i = 0; i < 4; i++) {
        Coordinate coord = coords[i] + getPosition();
        if (coord.getX() < 0 || coord.getX() >= BOARD_WIDTH || coord.getY() < 0 ||
            coord.getY() >= BOARD_HEIGHT || grid.isOccupied(coord)) {
            return true;
        }
    }
    return false;
}

void Piece::placeOn(Grid &grid) const {
    const Coordinate *coords = getRotationCoords();
    for (int i = 0; i < 4; i++) {
        Coordinate coord = coords[i] + getPosition();
        grid.setOccupied(coord, true);
        grid.setTileColor(coord, getColor());
    }
}

bool Piece::isOutOfBounds() const {
    const Coordinate *coords = getRotationCoords();
    for (int i = 0; i < 4; i++) {
        Coordinate coord = coords[i] + getPosition();
        if (coord.getX() < 0 || coord.getX() >= BOARD_WIDTH || coord.getY() < 0 ||
            coord.getY() >= BOARD_HEIGHT) {
            return true;
        }
    }
    return false;
}

TileColor Piece::getColor() const {
    switch (getType()) {
    case PieceType::I:
        return CYAN;
    case PieceType::J:
        return BLUE;
    case PieceType::L:
        return WHITE; // Devrait être orange, mais il n'y a pas d'orange par défaut.
    case PieceType::O:
        return YELLOW;
    case PieceType::S:
        return GREEN;
    case PieceType::T:
        return MAGENTA;
    case PieceType::Z:
        return RED;
    case PieceType::CUSTOM:
        return WHITE;
    }
    return BLACK;
}
} // namespace tetris
