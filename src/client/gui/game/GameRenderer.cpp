#include "GameRenderer.hpp"
#include "../engine/SFMLEngine.hpp"

namespace tetris::gui {

GameRenderer::GameRenderer(tetris::gui::SFMLEngine &engine) : engine(engine) {
    // Initialisation
    cellSize = 30;
    gridOffsetX = 50; // Grille plus à gauche
    gridOffsetY = 50;
}

void GameRenderer::render() {
    // Effacer l'écran
    engine.clear();

    // Fond
    engine.drawRectangle(0, 0, 800, 600, sf::Color(20, 20, 20));

    // Dessiner la grille et l'UI
    drawGrid();
    drawUI();

    // Afficher à l'écran
    engine.display();
}

void GameRenderer::drawGrid() {
    // Dessiner les bordures
    engine.drawRectangle(gridOffsetX - 2, gridOffsetY - 2, cellSize * 10 + 4, cellSize * 20 + 4,
                         sf::Color(100, 100, 100));

    // Dessiner les cellules
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 20; j++) {
            engine.drawRectangle(gridOffsetX + i * cellSize, gridOffsetY + j * cellSize,
                                 cellSize - 1, cellSize - 1, sf::Color(40, 40, 40));
        }
    }
}

void GameRenderer::drawUI() {
    // Interface utilisateur à droite de la grille
    int uiX = gridOffsetX + 10 * cellSize + 50; // Décalage par rapport à la grille

    // Titre du jeu
    engine.drawText("TETRIS ROYALE", uiX, 50, 28, sf::Color::Yellow);

    // Informations de jeu
    engine.drawText("Score: 0", uiX, 100, 24, sf::Color::White);
}

} // namespace tetris::gui