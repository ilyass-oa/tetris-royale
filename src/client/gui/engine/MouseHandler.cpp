#include "MouseHandler.hpp"

namespace tetris::gui {

MouseHandler::MouseHandler()
    : isLeftButtonPressed(false), isRightButtonPressed(false), wasLeftButtonPressed(false),
      wasRightButtonPressed(false) {}

void MouseHandler::update(const sf::Event &event) {
    if (event.is<sf::Event::MouseMoved>()) {
        if (const auto *mouseEvent = event.getIf<sf::Event::MouseMoved>()) {
            mousePosition = {mouseEvent->position.x, mouseEvent->position.y};
        }
    } else if (event.is<sf::Event::MouseButtonPressed>()) {
        if (const auto *mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mouseEvent->button == sf::Mouse::Button::Left) {
                isLeftButtonPressed = true;
            } else if (mouseEvent->button == sf::Mouse::Button::Right) {
                isRightButtonPressed = true;
            }
        }
    } else if (event.is<sf::Event::MouseButtonReleased>()) {
        if (const auto *mouseEvent = event.getIf<sf::Event::MouseButtonReleased>()) {
            if (mouseEvent->button == sf::Mouse::Button::Left) {
                isLeftButtonPressed = false;
            } else if (mouseEvent->button == sf::Mouse::Button::Right) {
                isRightButtonPressed = false;
            }
        }
    }
}

// Met à jour l'état des boutons pour la frame suivante
void MouseHandler::updateFrame() {
    wasLeftButtonPressed = isLeftButtonPressed;
    wasRightButtonPressed = isRightButtonPressed;
}

// Getters
sf::Vector2i MouseHandler::getMousePosition() const {
    return mousePosition;
}

bool MouseHandler::isLeftClick() const {
    return isLeftButtonPressed && !wasLeftButtonPressed;
}

bool MouseHandler::isRightClick() const {
    return isRightButtonPressed && !wasRightButtonPressed;
}

// Vérifie si la souris est dans un rectangle
bool MouseHandler::isInRectangle(float rectX, float rectY, float rectWidth,
                                 float rectHeight) const {
    return (mousePosition.x >= rectX && mousePosition.x <= rectX + rectWidth &&
            mousePosition.y >= rectY && mousePosition.y <= rectY + rectHeight);
}

// Vérifie si on a cliqué dans un rectangle
bool MouseHandler::clickedInRectangle(float rectX, float rectY, float rectWidth,
                                      float rectHeight) const {
    return isLeftClick() && isInRectangle(rectX, rectY, rectWidth, rectHeight);
}

} // namespace tetris::gui
