#ifndef GAMEINTERFACE_HPP
#define GAMEINTERFACE_HPP

#include "GameInputHandler.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <game/TetrisGame.hpp>
#include <memory>
#include <vector>

// Déclaration anticipée (forward declaration)
namespace tetris {
namespace gui {
class SFMLEngine;   // Déclaration anticipée
enum class AppMode; // Déclaration anticipée de l'enum qui est défini dans SFMLEngine.hpp
} // namespace gui
} // namespace tetris

namespace tetris::gui {

// Structure for multi-grid positioning and sizing
struct GridLayout {
    int offsetX;
    int offsetY;
    int width;
    int height;
    int tileSize;
};

class GameInterface {
  private:
    tetris::gui::SFMLEngine &engine;
    sf::RenderWindow *window;

    // Variables pour le timing
    static constexpr float GAME_UPDATE_RATE = 0.033f; // ~30Hz

    // Variables du jeu
    std::shared_ptr<tetris::TetrisGame> game;
    std::unique_ptr<GameInputHandler> inputHandler;
    sf::Clock fallClock;
    int totalLinesCleared = 0; // Compteur global des lignes complétées
    int tileSize = 25;         // Taille d'une case en pixels
    int gridOffsetX = 50;      // Position X de la grille
    int gridOffsetY = 50;      // Position Y de la grille
    bool gameOver = false;
    bool stop_speed_increase = false;
    int fall_delay = 500;     // Délai de chute en millisecondes
    int min_fall_delay = 100; // Délai de chute minimum

  public:
    GameInterface(tetris::gui::SFMLEngine &engine, sf::RenderWindow *window);

    void update(float deltaTime);
    void render();
    void handleEvents(const sf::Event &event);
    void speed_up(int ms);
    void reset();

    // Méthodes de rendu
    void drawGrid();
    void drawCurrentPiece();
    void drawNextPiece();
    void drawSideBar();

    // Méthodes de rendu avec données réseau
    void renderWithNetworkData(const std::vector<std::vector<int>> &gridData, int score,
                               tetris::PieceType nextPiece, bool isGameOver, int offsetX = -1,
                               int offsetY = -1, int width = -1, int height = -1,
                               const std::string &statisticLabel = "SCORE");
    void drawNetworkGrid(const std::vector<std::vector<int>> &gridData, int offsetX = -1,
                         int offsetY = -1, int width = -1, int height = -1,
                         int customTileSize = -1);
    void drawNetworkNextPiece(tetris::PieceType nextPiece, int offsetX = -1, int offsetY = -1,
                              float scale = 1.0f);
    void drawNetworkSideBar(int score, int offsetX = -1, int offsetY = -1,
                            const std::string &label = "SCORE");

    // Méthodes pour le multi-joueur
    void renderMultiplayerGrids(const std::vector<std::vector<std::vector<int>>> &grids,
                                const std::vector<GridLayout> &layouts);
    int calculateOptimalTileSize(int numGrids, int screenWidth, int screenHeight) const;
    std::vector<GridLayout> calculateGridLayouts(int numGrids, int screenWidth,
                                                 int screenHeight) const;

    // Convertir les couleurs internes en couleurs SFML
    sf::Color getTileColor(tetris::TileColor color);
    sf::Color getColorFromCode(int colorCode);

    // Méthode pour vérifier si un retour au menu est demandé
    bool wantsToReturnToMenu() const;
};

} // namespace tetris::gui

#endif // GAMEINTERFACE_HPP
