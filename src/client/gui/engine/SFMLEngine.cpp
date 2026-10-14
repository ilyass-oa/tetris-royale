#include "SFMLEngine.hpp"
#include "../menu/Menu.hpp" // Include Menu
#include "SFML/Window/Event.hpp"

#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace tetris::gui {

void SFMLEngine::must_init(bool v, const char *error_msg) {
    if (v) {
        return;
    }
    throw std::runtime_error(error_msg);
}

void SFMLEngine::must_init(void *v, const char *error_msg) {
    must_init(v != NULL, error_msg);
}

SFMLEngine::SFMLEngine(int windowWidth, int windowHeight, int refreshRate, const std::string &title)
    : refreshRate(refreshRate) {

    // Create window with Vector2u format
    window = new sf::RenderWindow(sf::VideoMode({static_cast<unsigned int>(windowWidth),
                                                 static_cast<unsigned int>(windowHeight)}),
                                  title);
    must_init(window, "failed to create window");
    window->setFramerateLimit(refreshRate);

    // Load default font
    font = new sf::Font();
    if (!font->openFromFile(ASSETS "/fonts/DejaVuSans.ttf")) {
        std::cerr << "Error: missing DejaVu Sans font; rebuild from a complete checkout."
                  << std::endl;
    }

    // Create timer
    timer = new sf::Clock();
    must_init(timer, "failed to create timer");
}

SFMLEngine::~SFMLEngine() {
    if (font) {
        delete font;
    }
    if (timer) {
        delete timer;
    }
    if (window) {
        window->close();
        delete window;
    }
}

// Main loop
void SFMLEngine::run() {
    bool done = false;
    float frameTime = 1.0f / refreshRate;
    float timeSinceLastUpdate = 0.0f;

    timer->restart();

    while (!done && window->isOpen()) {
        // Calculer le delta time
        float dt = timer->restart().asSeconds();
        timeSinceLastUpdate += dt;

        // Traiter les événements
        std::optional<sf::Event> eventOpt;
        while ((eventOpt = window->pollEvent())) {
            const sf::Event &event = *eventOpt;

            if (event.is<sf::Event::Closed>()) {
                if (event.getIf<sf::Event::Closed>()) {
                    window->close();
                    done = true;
                }
            }

            // Mise à jour du MouseHandler
            mouseHandler.update(event);

            // Traiter les événements selon le mode actuel
            if (currentMode == AppMode::Menu && menu) {
                menu->handleEvents(event);
            } else if (currentMode == AppMode::Game && gameInterface) {
                gameInterface->handleEvents(event);
            }
        }

        // Mettre à jour la logique avec un pas de temps fixe
        while (timeSinceLastUpdate >= frameTime) {
            // Mise à jour selon le mode actuel
            if (currentMode == AppMode::Menu && menu) {
                menu->update(frameTime);
            } else if (currentMode == AppMode::Game && gameInterface) {
                gameInterface->update(frameTime);
            }

            timeSinceLastUpdate -= frameTime;
        }

        // Rendu
        window->clear(sf::Color(40, 40, 40));

        // Rendu selon le mode actuel
        if (currentMode == AppMode::Menu && menu) {
            menu->render();
        } else if (currentMode == AppMode::Game && gameInterface) {
            gameInterface->render();
        }

        window->display();
    }
}

// Méthode pour associer une interface de jeu
void SFMLEngine::setGameInterface(tetris::gui::GameInterface *interface) {
    this->gameInterface = interface;
}

// Méthode pour associer un menu
void SFMLEngine::setMenu(Menu *menu) {
    this->menu = menu;
}

// Méthode pour définir le mode d'application
void SFMLEngine::setAppMode(AppMode mode) {
    this->currentMode = mode;
}

// Méthode pour obtenir le mode actuel
AppMode SFMLEngine::getAppMode() const {
    return currentMode;
}

bool SFMLEngine::isWindowOpen() const {
    return window && window->isOpen();
}

void SFMLEngine::handleEvents() {
// Assure que tout les evenements sont bien traités sur macOS
#ifdef __APPLE__
    window->setActive(true);
#endif

    std::optional<sf::Event> eventOpt;
    while ((eventOpt = window->pollEvent())) {
        const sf::Event &event = *eventOpt;

        if (event.is<sf::Event::Closed>()) {
            if (event.getIf<sf::Event::Closed>()) {
                window->close();
            }
        }

        // Mise à jour du MouseHandler
        mouseHandler.update(event);

        // Traiter les événements selon le mode actuel
        if (currentMode == AppMode::Menu && menu) {
            menu->handleEvents(event);
        } else if (currentMode == AppMode::Game && gameInterface) {
            gameInterface->handleEvents(event);
        }
    }

    // Mise à jour de l'état des boutons pour la frame suivante
    mouseHandler.updateFrame();
}

void SFMLEngine::clear() {
    if (window) {
        window->clear(sf::Color(40, 40, 40));
    }
}

void SFMLEngine::drawRectangle(float x, float y, float width, float height,
                               const sf::Color &color) {
    sf::RectangleShape rectangle({width, height});
    rectangle.setPosition({x, y});
    rectangle.setFillColor(color);
    window->draw(rectangle);
}

void SFMLEngine::drawText(const std::string &text, float x, float y, unsigned int size,
                          const sf::Color &color) {
    if (!font)
        return;

    sf::Text sfText(*font, text, size);
    sfText.setPosition({x, y});
    sfText.setFillColor(color);
    window->draw(sfText);
}

void SFMLEngine::drawImage(const std::string &imagePath, float x, float y, float width,
                           float height) {
    sf::Texture texture;
    if (!texture.loadFromFile(imagePath)) {
        std::cerr << "Erreur : impossible de charger l'image " << imagePath << std::endl;
        return;
    }

    sf::Sprite sprite(texture);

    // Calcul de l'échelle à appliquer pour atteindre la taille voulue
    sf::Vector2u textureSize = texture.getSize();
    if (textureSize.x == 0 || textureSize.y == 0)
        return;

    sprite.setPosition({x, y});
    sprite.setScale({width / textureSize.x, height / textureSize.y});

    window->draw(sprite);
}

void SFMLEngine::loadBackground(const std::string &path, float width, float height) {
    if (backgroundTexture.loadFromFile(path)) {
        backgroundSprite.emplace(backgroundTexture);

        float scaleX = static_cast<float>(width) / backgroundTexture.getSize().x;
        float scaleY = static_cast<float>(height) / backgroundTexture.getSize().y;
        backgroundSprite->setScale(sf::Vector2f(scaleX, scaleY));

        hasBackground = true;
    } else {
        std::cerr << "Erreur : impossible de charger le fond depuis " << path << std::endl;
    }
}

void SFMLEngine::drawBackground() {
    if (hasBackground && backgroundSprite.has_value()) {
        window->draw(backgroundSprite.value());
    }
}

void SFMLEngine::display() {
    if (window) {
        window->display();
    }
}

sf::RenderWindow *SFMLEngine::getWindow() {
    return window;
}

void SFMLEngine::drawIcon(const std::string &imagePath, float x, float y, float width,
                          float height) {
    sf::Texture texture;
    if (!texture.loadFromFile(imagePath)) {
        std::cerr << "Erreur : impossible de charger l'icon " << imagePath << std::endl;
        return;
    }
    sf::Sprite sprite(texture);

    sf::Vector2u textureSize = texture.getSize();
    if (textureSize.x == 0 || textureSize.y == 0)
        return;

    sprite.setPosition({x, y});
    sprite.setScale({width / textureSize.x, height / textureSize.y});
    window->draw(sprite);
}

} // namespace tetris::gui
