#ifndef SFMLENGINE_HPP
#define SFMLENGINE_HPP

#include "../game/GameInterface.hpp"
#include "MouseHandler.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <optional>
#include <string>

// Forward declarations
namespace tetris::gui {
class GameInterface; // Déclaration anticipée
}

class Menu; // Forward declaration for Menu class

namespace tetris::gui {

enum class AppMode { Menu, Game }; // Mode de l'application (menu ou jeu)

class SFMLEngine {
  private:
    sf::RenderWindow *window;
    sf::Font *font;

    sf::Clock *timer;
    sf::Texture backgroundTexture;
    std::optional<sf::Sprite> backgroundSprite;
    bool hasBackground = false;
    int refreshRate;
    MouseHandler mouseHandler;

    // Interface du jeu et menu
    GameInterface *gameInterface = nullptr;
    Menu *menu = nullptr;                // Référence au menu
    AppMode currentMode = AppMode::Menu; // Mode par défaut

    // Error checking helper
    void must_init(bool v, const char *error_msg);
    void must_init(void *v, const char *error_msg);

  public:
    SFMLEngine(int windowWidth, int windowHeight, int refreshRate, const std::string &title);

    ~SFMLEngine();

    void run();

    // Accesseurs
    sf::RenderWindow *getWindow();
    void setGameInterface(GameInterface *interface);
    void setMenu(Menu *menu);      // Méthode pour définir le menu
    void setAppMode(AppMode mode); // Méthode pour changer le mode
    AppMode getAppMode() const;    // Méthode pour obtenir le mode actuel

    // Méthodes ajoutées pour compatibilité avec main.cpp
    bool isWindowOpen() const;
    void handleEvents();
    void clear();
    void drawRectangle(float x, float y, float width, float height, const sf::Color &color);
    void drawText(const std::string &text, float x, float y, unsigned int size,
                  const sf::Color &color);
    void drawImage(const std::string &imagePath, float x, float y, float width, float height);
    void loadBackground(const std::string &path, float width, float height);
    void drawBackground();
    void display();
    void drawIcon(const std::string &imagePath, float x, float y, float width, float height);
    // Méthodes pour la souris
    const MouseHandler &getMouseHandler() const {
        return mouseHandler;
    }
};

} // namespace tetris::gui

#endif // SFMLENGINE_HPP
