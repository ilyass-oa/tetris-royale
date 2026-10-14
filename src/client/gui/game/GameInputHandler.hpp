#ifndef GAMEINPUTHANDLER_HPP
#define GAMEINPUTHANDLER_HPP

#include "constants.h"
#include <SFML/Window/Keyboard.hpp>
#include <game/TetrisGame.hpp>
#include <memory>

namespace tetris::gui {

// Classe gérant les entrées du jeu
class GameInputHandler {
  private:
    std::shared_ptr<tetris::TetrisGame> game;
    void hardDrop();
    bool networkMode = false;

  public:
    GameInputHandler(std::shared_ptr<tetris::TetrisGame> game);
    void handleKeyPressed(sf::Keyboard::Key key);

    // Network mode methods
    void setNetworkMode(bool enabled);
    void sendInputToServer(sf::Keyboard::Key key);
    int convertKeyToServerCode(sf::Keyboard::Key key);

    bool returnToMenu = false; // indique si on veut retourner au menu
};

} // namespace tetris::gui

#endif // GAMEINPUTHANDLER_HPP