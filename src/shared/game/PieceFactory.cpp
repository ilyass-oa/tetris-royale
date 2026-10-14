#include <algorithm>
#include <chrono>
#include <game/PieceFactory.hpp>
#include <random>

namespace tetris {

PieceFactory::PieceFactory() {
    // Initialiser le générateur de nombres aléatoires avec une graine basée sur l'heure actuelle
    auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    rng = std::default_random_engine(seed);
    allPieces = {I, O, T, S, Z, J, L};
    refillBag();
}

std::unique_ptr<Piece> PieceFactory::createOneByOnePiece() {
    return std::make_unique<Piece>(PieceType::CUSTOM, Coordinate(4, 0));
}

void PieceFactory::refillBag() {
    bag = allPieces; // Copier les pièces
    std::shuffle(bag.begin(), bag.end(),
                 rng); // Mélanger le bag avec le générateur de nombres aléatoires
}

PieceType PieceFactory::drawPiece() {
    if (bag.empty()) {
        refillBag();
    }
    PieceType piece = bag.back();
    bag.pop_back();
    return piece;
}

PieceType PieceFactory::peekNextPiece() {
    if (bag.empty()) {
        refillBag();
    }
    return bag.back();
}

std::unique_ptr<Piece> PieceFactory::createPiece() {
    return std::make_unique<Piece>(drawPiece(), Coordinate(4, 0));
}
} // namespace tetris