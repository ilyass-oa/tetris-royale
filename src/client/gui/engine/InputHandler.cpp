#include "InputHandler.hpp"

InputHandler::InputHandler(float x, float y, float width, float height, const std::string &label,
                           bool isPassword, const std::string &backgroundImage, float textOffsetX)
    : x(x), y(y), width(width), height(height), label(label), passwordMode(isPassword),
      focused(false), cursorVisible(false), backgroundImage(backgroundImage),
      textOffsetX(textOffsetX) {
    blinkClock.restart();
}

void InputHandler::handleEvent(const sf::Event &event, const tetris::gui::MouseHandler &mouse) {
    // Gestion du clic pour focus
    if (mouse.clickedInRectangle(x, y, width, height)) {
        focused = true;
        cursorVisible = true;
        blinkClock.restart();
        showErrorMessage = false;
    }

    if (event.is<sf::Event::MouseButtonPressed>()) {
        // Si clic ailleurs, enlever le focus
        if (!mouse.isInRectangle(x, y, width, height)) {
            focused = false;
        }
    }

    if (focused && event.is<sf::Event::TextEntered>()) {
        if (const auto *textEvent = event.getIf<sf::Event::TextEntered>()) {
            char c = static_cast<char>(textEvent->unicode);
            if (c == '\b') {
                if (!value.empty())
                    value.pop_back();
            } else if (c >= 32 && c < 127) {
                value += c;
            }
            cursorVisible = true;
            blinkClock.restart();
        }
    }
}

void InputHandler::update() {
    if (focused && blinkClock.getElapsedTime().asSeconds() > 0.5f) {
        cursorVisible = !cursorVisible;
        blinkClock.restart();
    }
}

void InputHandler::render(tetris::gui::SFMLEngine &engine) {
    // Draw the input field background
    if (!backgroundImage.empty()) {
        engine.drawImage(backgroundImage, x, y, width, height);
    } else {
        engine.drawRectangle(x, y, width, height, sf::Color::White);
    }

    std::string displayText =
        showErrorMessage ? errorMessage : (passwordMode ? std::string(value.size(), '*') : value);
    engine.drawText(displayText, x + textOffsetX, y + 10, 20, sf::Color::Black);

    // Draw cursor
    if (focused && cursorVisible && showErrorMessage) {
        std::string textBeforeCursor = passwordMode ? std::string(value.size(), '*') : value;
        // Estimate text width based on character count (approximately 10 pixels per character)
        float estimatedTextWidth = textBeforeCursor.length() * 10;
        float cursorX = x + textOffsetX + estimatedTextWidth;
        engine.drawRectangle(cursorX, y + 10, 2, 20, sf::Color::Black);
    }
}

std::string InputHandler::getText() const {
    return value;
}

void InputHandler::setText(const std::string &text) {
    value = text;
}

void InputHandler::setFocus(bool focus) {
    focused = focus;
}

bool InputHandler::isFocused() const {
    return focused;
}

void InputHandler::setErrorMessage(const std::string &message) {
    errorMessage = message;
    showErrorMessage = true;
}

void InputHandler::setPasswordHidden(bool hidden) {
    passwordMode = hidden;
}
void InputHandler::setGeometry(float x, float y, float width, float height) {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
}