#ifndef MOUSEHANDLER_HPP
#define MOUSEHANDLER_HPP

#include <SFML/Window/Event.hpp>

namespace tetris::gui {

class MouseHandler {
  private:
    sf::Vector2i mousePosition;
    bool isLeftButtonPressed;
    bool isRightButtonPressed;
    bool wasLeftButtonPressed;
    bool wasRightButtonPressed;

  public:
    MouseHandler();

    void update(const sf::Event &event);
    void updateFrame();

    // Getters
    sf::Vector2i getMousePosition() const;
    // Vérifie si le bouton gauche est cliqué
    bool isLeftClick() const;
    // Vérifie si le bouton droit est cliqué
    bool isRightClick() const;

    // Vérifie si la souris est dans un rectangle
    bool isInRectangle(float rectX, float rectY, float rectWidth, float rectHeight) const;

    // Vérifie si on a cliqué dans un rectangle
    bool clickedInRectangle(float rectX, float rectY, float rectWidth, float rectHeight) const;
};

} // namespace tetris::gui

#endif // MOUSEHANDLER_HPP
