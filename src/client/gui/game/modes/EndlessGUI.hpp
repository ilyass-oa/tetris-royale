#ifndef ENDLESSGUI_HPP
#define ENDLESSGUI_HPP

#include "../GameInputHandler.hpp"
#include "../GameInterface.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <game/TetrisGame.hpp>
#include <memory>

namespace tetris::gui {

class EndlessGUI {
  private:
    GameInterface gameInterface;
    sf::RenderWindow *window;
    bool running;
    bool gameOver;

  public:
    EndlessGUI(tetris::gui::SFMLEngine &engine, sf::RenderWindow *window);
    ~EndlessGUI();

    void update(float deltaTime);
    void render();
    void handleEvents(const sf::Event &event);
    bool wantsToReturnToMenu() const;
    void start();
    void stop();
};

} // namespace tetris::gui

#endif // ENDLESSGUI_HPP