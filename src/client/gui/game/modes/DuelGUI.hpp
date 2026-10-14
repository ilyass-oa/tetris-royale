#ifndef DUELGUI_HPP
#define DUELGUI_HPP

#include <SFML/Graphics.hpp>
#include <memory>
#include <packets_include.hpp>
#include <vector>

// Forward declarations
namespace tetris::gui {
class SFMLEngine;
class ClassicGUI;
} // namespace tetris::gui

namespace tetris::gui {

class DuelGUI {
  private:
    sf::RenderWindow *window;
    tetris::gui::SFMLEngine &engine;

    // Instead of implementing everything, we'll use ClassicGUI
    std::unique_ptr<ClassicGUI> classicMode;

  public:
    DuelGUI(tetris::gui::SFMLEngine &engine, sf::RenderWindow *window);
    ~DuelGUI();

    // Principales fonctions d'interface
    void start();
    void stop();
    void update(float deltaTime);
    void render();
    void handleEvents(const sf::Event &event);
    bool wantsToReturnToMenu() const;

    // Fonctions spécifiques au mode multijoueur
    void handlePlayerElimination(int playerIndex);

    // Fonctions de mise à jour des données
    void refresh_next_piece(tetris::PieceType newPiece);
};

} // namespace tetris::gui

#endif // DUELGUI_HPP