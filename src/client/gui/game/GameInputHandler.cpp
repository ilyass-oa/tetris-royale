#include "GameInputHandler.hpp"
#include "../../network/GlobalConnexion.hpp"
#include <key_codes.hpp>

namespace tetris::gui {

// Implémentation de GameInputHandler
GameInputHandler::GameInputHandler(std::shared_ptr<tetris::TetrisGame> game) : game(game) {
    returnToMenu = false;
    networkMode = false;
}

void GameInputHandler::setNetworkMode(bool enabled) {
    networkMode = enabled;
}

void GameInputHandler::handleKeyPressed(sf::Keyboard::Key key) {
    if (networkMode) {
        sendInputToServer(key);
        return;
    }

    // Gestion des touches pour déplacer/tourner les pièces
    switch (key) {
    case sf::Keyboard::Key::Left:
        game->moveCurrentPiece(-1, 0); // Déplacer à gauche
        break;
    case sf::Keyboard::Key::Right:
        game->moveCurrentPiece(1, 0); // Déplacer à droite
        break;
    case sf::Keyboard::Key::Down:
        game->moveCurrentPiece(0, 1); // Déplacer vers le bas
        break;
    case sf::Keyboard::Key::Up:
    case sf::Keyboard::Key::X:
        game->rotateCurrentPieceClockwise(); // Rotation sens horaire
        break;
    case sf::Keyboard::Key::Z:
        game->rotateCurrentPieceCounterclockwise(); // Rotation sens anti-horaire
        break;
    case sf::Keyboard::Key::C:
        game->rotateCurrentPiece180();
        break;
    case sf::Keyboard::Key::Space:
        hardDrop(); // Chute rapide
        break;
    case sf::Keyboard::Key::Q:
        game->end();
        returnToMenu = true; // Retourner au menu principal
        break;
    default:
        break;
    }
}

int GameInputHandler::convertKeyToServerCode(sf::Keyboard::Key key) {
    // Convert SFML keys to server input codes
    switch (key) {
    case sf::Keyboard::Key::Left:
        return input::left;
    case sf::Keyboard::Key::Right:
        return input::right;
    case sf::Keyboard::Key::Down:
        return input::down;
    case sf::Keyboard::Key::Up:
        return 'x'; // Rotate clockwise
    case sf::Keyboard::Key::Z:
        return 'z'; // Rotate counter-clockwise
    case sf::Keyboard::Key::X:
        return 'x'; // Rotate clockwise
    case sf::Keyboard::Key::C:
        return 'c'; // Rotate 180
    case sf::Keyboard::Key::Space:
        return ' ';
    case sf::Keyboard::Key::Q:
        return 'q'; // Quit
    default:
        return 0; // No mapping
    }
}

void GameInputHandler::sendInputToServer(sf::Keyboard::Key key) {
    int serverCode = convertKeyToServerCode(key);

    if (serverCode == 'q') {
        // Mark for return to menu
        returnToMenu = true;
    }

    if (serverCode != 0) {
        tetris::GlobalConnection::getInstance().send_input(serverCode);
    }
}

void GameInputHandler::hardDrop() {
    game->hardDrop();
    game->checkForLines();
}

} // namespace tetris::gui
