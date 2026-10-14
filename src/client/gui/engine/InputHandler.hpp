#ifndef INPUTHANDLER_HPP
#define INPUTHANDLER_HPP

#include "MouseHandler.hpp"
#include "SFMLEngine.hpp"
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <optional>
#include <string>

class InputHandler {
  public:
    InputHandler(float x, float y, float width, float height, const std::string &label,
                 bool isPassword = false, const std::string &backgroundImage = "",
                 float textOffsetX = 10);

    void handleEvent(const sf::Event &event, const tetris::gui::MouseHandler &mouse);
    void render(tetris::gui::SFMLEngine &engine);
    void update();

    std::string getText() const;
    void setText(const std::string &text);

    void setFocus(bool focus);
    bool isFocused() const;

    void setErrorMessage(const std::string &message);
    void setPasswordHidden(bool hidden);
    void setGeometry(float x, float y, float width, float height);

  private:
    float x, y, width, height;
    std::string label;
    std::string value;
    bool passwordMode;
    bool focused;
    bool cursorVisible;
    sf::Clock blinkClock;
    std::string backgroundImage;
    float textOffsetX;
    std::string errorMessage;
    bool showErrorMessage;
};

#endif // INPUTHANDLER_HPP
