#include <algorithm>
#include <game/TetrisGame.hpp>

// Classe qui gère la logique de base du jeu. Elle ne gère pas l'execution du jeu, mais les règles
// du jeu.
namespace tetris {
// Basé sur les offsets de Guideline
// Les valeurs Y sont inversées car dans le cadre de ce jeu, Y positif représente le bas
// https://tetris.wiki/Super_Rotation_System#How_Guideline_SRS_Really_Works
static const Coordinate I_PIECE_OFFSETS[4][5] = {
    /* 0 */ {Coordinate(0, 0), Coordinate(-1, 0), Coordinate(2, 0), Coordinate(-1, 0),
             Coordinate(2, 0)},
    /* R */
    {Coordinate(-1, 0), Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, -1), Coordinate(0, 2)},
    /* 2 */
    {Coordinate(-1, -1), Coordinate(1, -1), Coordinate(-2, -1), Coordinate(1, 0),
     Coordinate(-2, 0)},
    /* L */
    {Coordinate(0, -1), Coordinate(0, -1), Coordinate(0, -1), Coordinate(0, 1), Coordinate(0, -2)}};

static const Coordinate O_PIECE_OFFSETS[4][5] = {
    /* 0 */ {Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, 0),
             Coordinate(0, 0)},
    /* R */
    {Coordinate(0, 1), Coordinate(0, 1), Coordinate(0, 1), Coordinate(0, 1), Coordinate(0, 1)},
    /* 2 */
    {Coordinate(-1, 1), Coordinate(-1, 1), Coordinate(-1, 1), Coordinate(-1, 1), Coordinate(-1, 1)},
    /* L */
    {Coordinate(-1, 0), Coordinate(-1, 0), Coordinate(-1, 0), Coordinate(-1, 0),
     Coordinate(-1, 0)}};

static const Coordinate OTHER_PIECE_OFFSETS[4][5] = {
    /* 0 */ {Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, 0),
             Coordinate(0, 0)},
    /* R */
    {Coordinate(0, 0), Coordinate(1, 0), Coordinate(1, 1), Coordinate(0, -2), Coordinate(1, -2)},
    /* 2 */
    {Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, 0), Coordinate(0, 0)},
    /* L */
    {Coordinate(0, 0), Coordinate(-1, 0), Coordinate(-1, 1), Coordinate(0, -2),
     Coordinate(-1, -2)}};

TetrisGame::TetrisGame() {
    pieceFactory = std::make_unique<PieceFactory>();
    currentPiece = pieceFactory->createPiece();
    grid = std::make_shared<Grid>(); // Utilisation de shared_ptr
}

std::shared_ptr<Grid> TetrisGame::getGrid() {
    return grid;
}

int TetrisGame::getScore() const {
    return score;
}

int TetrisGame::getEnergyLevel() const {
    return energy;
}

void TetrisGame::incrementScore(int lines) {
    switch (lines) {
    case 1:
        score += 100;
        energy += 1;
        break;
    case 2:
        score += 300;
        energy += 2;
        break;
    case 3:
        score += 500;
        energy += 3;
        break;
    case 4:
        score += 800;
        energy += 5;
        break;
    }
    totalLinesCleared += lines;
}

void TetrisGame::applyKick() {
    const Coordinate *aOffsets;
    const Coordinate *bOffsets;

    switch (currentPiece->getType()) {
    case PieceType::I:
        aOffsets = I_PIECE_OFFSETS[currentPiece->getPrevRotation()];
        bOffsets = I_PIECE_OFFSETS[currentPiece->getRotation()];
        break;
    case PieceType::O:
        aOffsets = O_PIECE_OFFSETS[currentPiece->getPrevRotation()];
        bOffsets = O_PIECE_OFFSETS[currentPiece->getRotation()];
        break;
    default:
        aOffsets = OTHER_PIECE_OFFSETS[currentPiece->getPrevRotation()];
        bOffsets = OTHER_PIECE_OFFSETS[currentPiece->getRotation()];
        break;
    }

    for (int i = 0; i < 5; i++) {
        Coordinate offset = aOffsets[i] - bOffsets[i];
        currentPiece->move(offset.getX(), offset.getY());
        if (!currentPiece->isOutOfBounds() && !currentPiece->collidesWith(*grid)) {
            return;
        }
        currentPiece->move(-offset.getX(), -offset.getY());
    }
    // Si aucun kick ne fonctionne, on annule la rotation
    currentPiece->revertRotation();
}

bool TetrisGame::isOccupied(Coordinate coord) const {
    return grid->isOccupied(coord);
}

TileColor TetrisGame::getColor(Coordinate coord) const {
    return grid->getTileColor(coord);
}

bool TetrisGame::containsCurrentPiece(Coordinate coord) const {
    return currentPiece->occupies(coord);
}

TileColor TetrisGame::getCurrentPieceColor() const {
    return currentPiece->getColor();
}

bool TetrisGame::getRunning() const {
    return isRunning;
}

void TetrisGame::end() {
    isRunning = false;
}

void TetrisGame::rotateCurrentPieceClockwise() {
    currentPiece->rotateClockwise();
    applyKick();
}

void TetrisGame::rotateCurrentPieceCounterclockwise() {
    currentPiece->rotateCounterclockwise();
    applyKick();
}

void TetrisGame::rotateCurrentPiece180() {
    currentPiece->rotate180();
    applyKick();
}

void TetrisGame::moveCurrentPiece(int dx, int dy) {
    currentPiece->move(dx, dy);
    if (currentPiece->isOutOfBounds() || currentPiece->collidesWith(*grid)) {
        currentPiece->move(-dx, -dy);
    }
}

void TetrisGame::placeCurrentPiece() {
    currentPiece->placeOn(*grid);
    dropGrace = true;

    if (oneByOneMode && oneByOneCount > 0) {
        currentPiece = pieceFactory->createOneByOnePiece();
        oneByOneCount--;
        if (oneByOneCount == 0) {
            oneByOneMode = false;
        }
    } else {
        currentPiece = pieceFactory->createPiece();
    }
    if (currentPiece->collidesWith(*grid)) {
        isRunning = false;
    }
}

void TetrisGame::checkForPlacement() {
    currentPiece->move(0, 1);
    if (currentPiece->isOutOfBounds() || currentPiece->collidesWith(*grid)) {
        currentPiece->move(0, -1);
        if (dropGrace) {
            dropGrace = false;
        } else {
            placeCurrentPiece();
        }
    } else {
        currentPiece->move(0, -1);
        dropGrace = true;
    }
}

int TetrisGame::checkForLines() {
    int lines = 0;
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        bool line = true;
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (!grid->isOccupied(Coordinate(j, i))) {
                line = false;
                break;
            }
        }
        if (line) {
            lines++;
            grid->moveLinesDown(i);
        }
    }
    incrementScore(lines);
    return lines;
}

void TetrisGame::applyGrayLines(int malusLines) {
    // Applique le malu GrayLines
    grid->addGrayLines(malusLines);
}

void TetrisGame::update() {
    // Déplacer la pièce d'un cran vers le bas
    moveCurrentPiece(0, 1);
    // Vérifier si la pièce doit être posée
    checkForPlacement();
}

void TetrisGame::reset() {
    grid = std::make_shared<Grid>();
    pieceFactory = std::make_unique<PieceFactory>();
    currentPiece = pieceFactory->createPiece();
    score = 0;
    energy = 0;
    totalLinesCleared = 0;
    isRunning = true;
    dropGrace = true;
    oneByOneMode = false;
    oneByOneCount = 0;
    controlsInverted = false;
    controlsBlocked = false;
    speedMultiplier = 1;
    fallDelay = 250;
}
void TetrisGame::hardDrop() {
    if (!isRunning)
        return;
    do {
        currentPiece->move(0, 1);
    } while (!currentPiece->isOutOfBounds() && !currentPiece->collidesWith(*grid));
    currentPiece->move(0, -1);
    placeCurrentPiece();
}

void TetrisGame::oneByOneBlocks() {
    oneByOneMode = true;
    oneByOneCount = 2;
}

void TetrisGame::setFallDelay(int delay) {
    fallDelay = delay;
}

PieceFactory *TetrisGame::getPieceFactory() const {
    return pieceFactory.get();
}
// permet de supprimer les i lignes du haut
void TetrisGame::clearTopLines(int numLines) {
    for (int i = 0; i < std::clamp(numLines, 0, BOARD_HEIGHT); ++i) {
        grid->clearRow(i);
    }
}

} // namespace tetris
