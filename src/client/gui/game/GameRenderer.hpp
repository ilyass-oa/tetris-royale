#ifndef GAMERENDERER_HPP
#define GAMERENDERER_HPP

// Déclaration anticipée
namespace tetris::gui {
class SFMLEngine;
}

namespace tetris::gui {

// Classe simplifiée pour le rendu
class GameRenderer {
  private:
    tetris::gui::SFMLEngine &engine;

    // Paramètres de la grille de jeu
    int cellSize;    // Taille d'une cellule (en pixels
    int gridOffsetX; // Position X de la grille
    int gridOffsetY; // Position Y de la grille

  public:
    GameRenderer(tetris::gui::SFMLEngine &engine);

    void render();
    void drawGrid();
    void drawUI();
};

} // namespace tetris::gui

#endif // GAMERENDERER_HPP