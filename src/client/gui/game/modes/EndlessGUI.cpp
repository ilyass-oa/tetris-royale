#include "EndlessGUI.hpp"
#include <iostream>

namespace tetris::gui {

EndlessGUI::EndlessGUI(tetris::gui::SFMLEngine &engine, sf::RenderWindow *window)
    : gameInterface(engine, window), window(window), running(false), gameOver(false) {}

EndlessGUI::~EndlessGUI() {
    stop();
}

void EndlessGUI::start() {
    // Reset state
    gameOver = false;
    running = true;

    // std::cout << "Starting Endless mode..." << std::endl;

    // Reset game interface
    gameInterface.reset();
}

void EndlessGUI::stop() {
    running = false;
}

void EndlessGUI::update(float deltaTime) {
    // Don't update if game over
    if (gameOver)
        return;

    // Update game interface
    gameInterface.update(deltaTime);

    // Check for menu return
    if (gameInterface.wantsToReturnToMenu()) {
        stop();
    }
}

void EndlessGUI::render() {
    // Clear the window with a dark background
    window->clear(sf::Color(20, 20, 20));

    // Draw game title
    gameInterface.render();
}

void EndlessGUI::handleEvents(const sf::Event &event) {
    if (event.is<sf::Event::KeyPressed>()) {
        if (const auto *keyEvent = event.getIf<sf::Event::KeyPressed>()) {
            if (gameOver) {
                if (keyEvent->code == sf::Keyboard::Key::Q) {
                    running = false;
                    stop();
                }
                return;
            }

            // Si la touche Q est pressée, retourner au menu
            if (keyEvent->code == sf::Keyboard::Key::Q) {
                running = false;
                stop();
                return;
            }
        }
    }

    // Relayer les autres événements à l'interface de jeu
    gameInterface.handleEvents(event);
}

bool EndlessGUI::wantsToReturnToMenu() const {
    return !running;
}

} // namespace tetris::gui