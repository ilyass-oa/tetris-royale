#include "GameInterface.hpp"
#include "../../network/GlobalConnexion.hpp"
#include "../engine/SFMLEngine.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace tetris::gui {

// Constructeur avec initialisation du jeu
GameInterface::GameInterface(tetris::gui::SFMLEngine &engine, sf::RenderWindow *window)
    : engine(engine), window(window) {
    reset();
}

void GameInterface::reset() {
    // Initialiser une nouvelle partie
    game = std::make_shared<tetris::TetrisGame>();
    // Initialiser le gestionnaire d'entrées
    inputHandler = std::make_unique<GameInputHandler>(game);
    // Initialiser le délai de chute
    game->setFallDelay(500);
    gameOver = false;
    // Réinitialiser le flag de retour au menu
    if (inputHandler) {
        inputHandler->returnToMenu = false;
    }
    fallClock.restart();
}

void GameInterface::update(float deltaTime) {
    if (gameOver)
        return;

    // Mettre à jour la logique du jeu à un intervalle régulier
    const int FALL_DELAY = 500;
    if (fallClock.getElapsedTime().asMilliseconds() >= FALL_DELAY) {
        fallClock.restart();
        game->update();

        // Vérifier si des lignes ont été complétées
        int linesCleared = game->checkForLines();

        // Mise à jour du statut du jeu
        if (!game->getRunning()) {
            gameOver = true;

            // Enregistrer le score
            try {
                tetris::GlobalConnection::getInstance().send_score(game->getScore());
            } catch (const std::exception &) {
            }
        }
    }
}

bool GameInterface::wantsToReturnToMenu() const {
    return inputHandler->returnToMenu;
}

void GameInterface::render() {
    // Position des éléments
    gridOffsetY = 30;

    // Dessin du fond pour l'arrière-plan du jeu
    engine.drawRectangle(gridOffsetX - 5, gridOffsetY - 5, BOARD_WIDTH * tileSize + 10,
                         BOARD_HEIGHT * tileSize + 10, sf::Color(40, 40, 40));

    drawGrid();
    drawCurrentPiece();
    drawNextPiece();
    drawSideBar();
}

// Nouvelle méthode pour afficher le jeu avec des données réseau
void GameInterface::renderWithNetworkData(const std::vector<std::vector<int>> &gridData, int energy,
                                          tetris::PieceType nextPiece, bool isGameOver, int offsetX,
                                          int offsetY, int width, int height,
                                          const std::string &statisticLabel) {
    // Use provided position or default
    int useOffsetX = (offsetX >= 0) ? offsetX : gridOffsetX;
    int useOffsetY = (offsetY >= 0) ? offsetY : gridOffsetY;

    // Calculate scaling based on provided dimensions
    float scale = 1.0f;
    int useWidth = BOARD_WIDTH * tileSize;
    int useHeight = BOARD_HEIGHT * tileSize;
    int scaledTileSize = tileSize;

    if (width > 0 && height > 0) {
        float scaleX = static_cast<float>(width) / useWidth;
        float scaleY = static_cast<float>(height) / useHeight;
        scale = std::min(scaleX, scaleY);

        useWidth = static_cast<int>(useWidth * scale);
        useHeight = static_cast<int>(useHeight * scale);
        scaledTileSize = static_cast<int>(tileSize * scale);
    }

    // Dessin du fond pour l'arrière-plan du jeu
    engine.drawRectangle(useOffsetX - 5, useOffsetY - 5, useWidth + 10, useHeight + 10,
                         sf::Color(40, 40, 40));

    // Draw grid with custom size
    drawNetworkGrid(gridData, useOffsetX, useOffsetY, useWidth, useHeight);

    // Adjust sidebar and next piece position based on main grid
    // Only draw sidebar and next piece if the grid is large enough
    if (useWidth > 100) { // Arbitrary minimum size
        // Calculate sidebar position to right of grid
        int sidebarX = useOffsetX + useWidth + 10;
        int sidebarY = useOffsetY;

        // Draw next piece preview (scaled down)
        drawNetworkNextPiece(nextPiece, sidebarX, sidebarY, scale);

        // Draw score information
        drawNetworkSideBar(energy, sidebarX, sidebarY + 80 * scale, statisticLabel);
    }

    // Affichage du message de fin de jeu si le jeu est terminé
    if (isGameOver) {
        // Fond semi-transparent
        sf::Color overlayColor(0, 0, 0, 200);
        engine.drawRectangle(useOffsetX, useOffsetY, useWidth, useHeight, overlayColor);

        // Scale font size based on grid size
        int gameOverFontSize = static_cast<int>(30 * scale);
        int subTextFontSize = static_cast<int>(18 * scale);

        // Only show text if font size is large enough to be readable
        if (gameOverFontSize >= 10) {
            // Message de fin de jeu centré
            engine.drawText("GAME OVER", useOffsetX + (useWidth / 2) - gameOverFontSize * 2.5f,
                            useOffsetY + (useHeight / 2) - gameOverFontSize, gameOverFontSize,
                            sf::Color::Red);
        }
    }
}

void GameInterface::drawGrid() {
    auto grid = game->getGrid();

    // Dessiner la grille (les cases vides et occupées)
    for (int x = 0; x < BOARD_WIDTH; x++) {
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            Coordinate c(x, y);
            // Dessiner la case avec sa couleur
            sf::Color color = getTileColor(grid->getTileColor(c));
            engine.drawRectangle(gridOffsetX + x * tileSize, gridOffsetY + y * tileSize,
                                 tileSize - 1, tileSize - 1, color);
        }
    }
}

// Updated method for drawing network grid with customizable size
void GameInterface::drawNetworkGrid(const std::vector<std::vector<int>> &gridData, int offsetX,
                                    int offsetY, int width, int height, int customTileSize) {
    // Utiliser les offsets par défaut si non spécifiés
    if (offsetX < 0)
        offsetX = gridOffsetX;
    if (offsetY < 0)
        offsetY = gridOffsetY;

    // Use default or custom tile size
    int actualTileSize = (customTileSize > 0) ? customTileSize : tileSize;

    // Calculate dimensions
    int gridWidth = BOARD_WIDTH;
    int gridHeight = BOARD_HEIGHT;

    // Calculate scaling if width/height are provided
    float scaleX = 1.0f;
    float scaleY = 1.0f;

    if (width > 0)
        scaleX = static_cast<float>(width) / (gridWidth * actualTileSize);
    if (height > 0)
        scaleY = static_cast<float>(height) / (gridHeight * actualTileSize);

    // Apply the smallest scale to maintain aspect ratio
    float scale = std::min(scaleX, scaleY);

    // Calculate the actual tile size after scaling
    int scaledTileSize = static_cast<int>(actualTileSize * scale);

    // Ensure we're drawing at the exact position requested
    /*std::cerr << "Drawing grid at: (" << offsetX << "," << offsetY
              << ") with size: " << width << "x" << height
              << ", scaled tile size: " << scaledTileSize << std::endl;*/

    // Draw a visible border around the grid area
    sf::RectangleShape border;
    border.setSize(sf::Vector2f(static_cast<float>(gridWidth * scaledTileSize + 4),
                                static_cast<float>(gridHeight * scaledTileSize + 4)));
    border.setPosition(
        sf::Vector2f(static_cast<float>(offsetX - 2), static_cast<float>(offsetY - 2)));
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(sf::Color(200, 200, 200)); // Brighter border for visibility
    border.setOutlineThickness(2.0f);
    window->draw(border);

    // First draw the background grid with visible borders
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            // Draw empty cells with a dark gray color and visible border
            sf::RectangleShape cell;
            cell.setSize(sf::Vector2f(static_cast<float>(scaledTileSize - 1),
                                      static_cast<float>(scaledTileSize - 1)));
            cell.setPosition(sf::Vector2f(static_cast<float>(offsetX + x * scaledTileSize),
                                          static_cast<float>(offsetY + y * scaledTileSize)));
            cell.setFillColor(sf::Color(20, 20, 20));
            cell.setOutlineColor(sf::Color(40, 40, 40));
            cell.setOutlineThickness(1.0f);
            window->draw(cell);
        }
    }

    // Then draw the actual game grid data on top
    if (!gridData.empty()) {
        for (int y = 0; y < gridHeight && y < gridData.size(); y++) {
            if (!gridData[y].empty()) {
                for (int x = 0; x < gridWidth && x < gridData[y].size(); x++) {
                    // Only skip empty cells (EMPTY_COLOR), ensure we draw gray lines (value 7)
                    if (gridData[y][x] != 8) { // Shared protocol: 8 is an empty cell.
                        // Obtenir la couleur à partir du code dans gridData
                        sf::Color color = getColorFromCode(gridData[y][x]);

                        // Dessiner la case colorée
                        sf::RectangleShape filledCell;
                        filledCell.setSize(sf::Vector2f(static_cast<float>(scaledTileSize - 2),
                                                        static_cast<float>(scaledTileSize - 2)));
                        filledCell.setPosition(
                            sf::Vector2f(static_cast<float>(offsetX + x * scaledTileSize + 1),
                                         static_cast<float>(offsetY + y * scaledTileSize + 1)));
                        filledCell.setFillColor(color);
                        window->draw(filledCell);
                    }
                }
            }
        }
    }
}

// Method to render multiple grids for multiplayer
void GameInterface::renderMultiplayerGrids(const std::vector<std::vector<std::vector<int>>> &grids,
                                           const std::vector<GridLayout> &layouts) {
    if (grids.size() != layouts.size())
        return;

    for (size_t i = 0; i < grids.size(); i++) {
        const auto &layout = layouts[i];
        drawNetworkGrid(grids[i], layout.offsetX, layout.offsetY, layout.width, layout.height,
                        layout.tileSize);
    }
}

// Calculate optimal tile size based on screen dimensions and number of grids
int GameInterface::calculateOptimalTileSize(int numGrids, int screenWidth, int screenHeight) const {
    // Determine layout (rows and columns)
    int cols = static_cast<int>(std::ceil(std::sqrt(numGrids)));
    int rows = static_cast<int>(std::ceil(static_cast<float>(numGrids) / cols));

    // Calculate maximum possible tile size
    int maxTileWidth = (screenWidth - 150) / (cols * BOARD_WIDTH);    // 150px for UI elements
    int maxTileHeight = (screenHeight - 100) / (rows * BOARD_HEIGHT); // 100px for spacing

    // Return the smaller of the two to maintain aspect ratio
    return std::max(5, std::min(maxTileWidth, maxTileHeight)); // Minimum 5px
}

// Calculate grid layouts for multiple players
std::vector<GridLayout> GameInterface::calculateGridLayouts(int numGrids, int screenWidth,
                                                            int screenHeight) const {
    std::vector<GridLayout> layouts;

    if (numGrids <= 0)
        return layouts;

    // Calculate optimal tile size
    int tileSize = calculateOptimalTileSize(numGrids, screenWidth, screenHeight);

    // Determine layout (rows and columns)
    int cols = static_cast<int>(std::ceil(std::sqrt(numGrids)));
    int rows = static_cast<int>(std::ceil(static_cast<float>(numGrids) / cols));

    // Calculate grid dimensions
    int gridWidth = tileSize * BOARD_WIDTH;
    int gridHeight = tileSize * BOARD_HEIGHT;

    // Calculate total width and height
    int totalWidth = cols * gridWidth + (cols - 1) * 20; // 20px spacing between grids
    int totalHeight = rows * gridHeight + (rows - 1) * 20;

    // Calculate starting position to center everything
    int startX = (screenWidth - totalWidth) / 2;
    int startY = (screenHeight - totalHeight) / 2;

    // Create layouts for each grid
    for (int i = 0; i < numGrids; i++) {
        int row = i / cols;
        int col = i % cols;

        GridLayout layout;
        layout.offsetX = startX + col * (gridWidth + 20);
        layout.offsetY = startY + row * (gridHeight + 20);
        layout.width = gridWidth;
        layout.height = gridHeight;
        layout.tileSize = tileSize;

        layouts.push_back(layout);
    }

    return layouts;
}

void GameInterface::drawCurrentPiece() {
    // Dessiner la pièce en mouvement
    for (int x = 0; x < BOARD_WIDTH; x++) {
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            Coordinate c(x, y);
            if (game->containsCurrentPiece(c)) {
                // Dessiner la pièce en cours avec sa couleur
                sf::Color color = getTileColor(game->getCurrentPieceColor());
                engine.drawRectangle(gridOffsetX + x * tileSize, gridOffsetY + y * tileSize,
                                     tileSize - 1, tileSize - 1, color);
            }
        }
    }
}

void GameInterface::drawNextPiece() {
    // Dessiner l'arrière-plan pour la prochaine pièce
    int nextPieceX = gridOffsetX + BOARD_WIDTH * tileSize + 20;
    int nextPieceY = gridOffsetY + 150;

    // Titre pour la prochaine pièce
    engine.drawText("PROCHAINE", nextPieceX, nextPieceY - 25, 18, sf::Color::White);

    // Fond pour la zone de prochaine pièce
    engine.drawRectangle(nextPieceX, nextPieceY, 4 * tileSize, 4 * tileSize, sf::Color(40, 40, 40));

    // Récupérer et dessiner la prochaine pièce
    auto nextPieceType = game->getPieceFactory()->peekNextPiece();
    auto nextPiece = std::make_unique<tetris::Piece>(nextPieceType, Coordinate(0, 0));
    auto shape = nextPiece->getShape();
    // Centrer la pièce dans sa zone
    int offsetX = (4 - shape[0].size()) / 2;
    int offsetY = (4 - shape.size()) / 2;

    for (size_t y = 0; y < shape.size(); y++) {
        for (size_t x = 0; x < shape[y].size(); x++) {
            if (shape[y][x] != 0) {
                sf::Color color = getTileColor(nextPiece->getColor());
                engine.drawRectangle(nextPieceX + (offsetX + x) * tileSize,
                                     nextPieceY + (offsetY + y) * tileSize, tileSize - 1,
                                     tileSize - 1, color);
            }
        }
    }
}

// Nouvelle méthode pour dessiner la prochaine pièce à partir des données réseau
void GameInterface::drawNetworkNextPiece(tetris::PieceType nextPieceType, int offsetX, int offsetY,
                                         float scale) {
    // Position pour la prochaine pièce
    int nextPieceX = (offsetX < 0) ? (gridOffsetX + BOARD_WIDTH * tileSize + 20) : offsetX;
    int nextPieceY = gridOffsetY + 150;

    // Adjust tile size based on scale
    int scaledTileSize = static_cast<int>(tileSize * scale);

    // Titre pour la prochaine pièce
    int scaledFontSize = std::max(10, static_cast<int>(18 * scale));
    engine.drawText("PROCHAINE", nextPieceX, nextPieceY - 25, 18, sf::Color::White);

    // Fond pour la zone de prochaine pièce
    /*int previewSize = 2 * scaledTileSize;
    engine.drawRectangle(nextPieceX, nextPieceY + 15,
                      previewSize, previewSize,
                      sf::Color(40, 40, 40));*/

    // Créer une pièce du type spécifié
    auto nextPiece = std::make_unique<tetris::Piece>(nextPieceType, Coordinate(0, 0));
    auto shape = nextPiece->getShape();

    // Centrer la pièce dans sa zone
    int pieceOffsetX = (4 - shape[0].size()) / 2;
    int pieceOffsetY = (4 - shape.size()) / 2;

    for (size_t y = 0; y < shape.size(); y++) {
        for (size_t x = 0; x < shape[y].size(); x++) {
            if (shape[y][x] != 0) {
                sf::Color color = getTileColor(nextPiece->getColor());
                engine.drawRectangle(nextPieceX + (pieceOffsetX + x) * scaledTileSize,
                                     nextPieceY + (pieceOffsetY + y) * scaledTileSize,
                                     scaledTileSize - 1, scaledTileSize - 1, color);
            }
        }
    }
}

void GameInterface::drawSideBar() {
    // Afficher les informations du jeu dans la zone à droite de la grille
    int sideBarX = gridOffsetX + BOARD_WIDTH * tileSize + 20;

    // Titre du jeu en haut
    engine.drawText("TETRIS", sideBarX, gridOffsetY, 24, sf::Color::Yellow);

    // Convertir le score en string et le mettre a 6 chiffres
    std::string scoreText = std::to_string(game->getScore());
    // std::string energyText = std::to_string(game->getEnergyLevel());
    /*while (energyText.length() < 5) {
        energyText = "0" + energyText;
    }*/
    while (scoreText.length() < 6) {
        scoreText = "0" + scoreText;
    }

    // Largeur du cadre
    int scoreBarWidth = 150;
    // energyBarWidth = 150;

    // Cadre pour le score et l'énergie
    engine.drawRectangle(sideBarX, gridOffsetY + 40, scoreBarWidth, 35, sf::Color(60, 60, 60));
    // engine.drawRectangle(sideBarX, gridOffsetY + 80, energyBarWidth, 35, sf::Color(60, 60, 60));

    // Afficher "SCORE : Valeur"
    engine.drawText("SCORE :  ", sideBarX + 5, gridOffsetY + 42, 18, sf::Color::White);
    engine.drawText("  " + scoreText, sideBarX + 75, gridOffsetY + 42, 18, sf::Color::White);

    // Afficher "ENERGY : Valeur"
    // engine.drawText("ENERGY : ", sideBarX + 5, gridOffsetY + 82, 18, sf::Color::White);
    // engine.drawText(" " + energyText, sideBarX + 90, gridOffsetY + 82, 18, sf::Color::White);

    // Instructions pour quitter et rejouer a la fin du jeu
    engine.drawText("Q : Quitter", sideBarX, gridOffsetY + 275, 16, sf::Color::Red);

    // Affichage du message de fin de jeu si le jeu est terminé
    if (gameOver) {
        // Fond semi-transparent
        sf::Color overlayColor(0, 0, 0, 200);
        engine.drawRectangle(gridOffsetX, gridOffsetY, BOARD_WIDTH * tileSize,
                             BOARD_HEIGHT * tileSize, overlayColor);

        // Message de fin de jeu centré
        engine.drawText("GAME OVER", gridOffsetX + (BOARD_WIDTH * tileSize / 2) - 80,
                        gridOffsetY + (BOARD_HEIGHT * tileSize / 2) - 30, 30, sf::Color::Red);

        engine.drawText("APPUYER SUR R", gridOffsetX + (BOARD_WIDTH * tileSize / 2) - 70,
                        gridOffsetY + (BOARD_HEIGHT * tileSize / 2) + 10, 18, sf::Color::White);

        // Afficher le score final
        engine.drawText("SCORE : " + std::to_string(game->getScore()),
                        gridOffsetX + (BOARD_WIDTH * tileSize / 2) - 60,
                        gridOffsetY + (BOARD_HEIGHT * tileSize / 2) + 40, 18, sf::Color::Yellow);
    }
}

// Nouvelle méthode pour dessiner la barre latérale avec les données réseau
void GameInterface::drawNetworkSideBar(int value, int offsetX, int offsetY,
                                       const std::string &label) {
    const int x = offsetX < 0 ? gridOffsetX + BOARD_WIDTH * tileSize + 20 : offsetX;
    const int y = offsetY < 0 ? gridOffsetY + 80 : offsetY;
    std::string number = std::to_string(value);
    if (number.size() < 5)
        number.insert(0, 5 - number.size(), '0');
    engine.drawRectangle(x, y, 150, 35, sf::Color(60, 60, 60));
    engine.drawText(label + " : " + number, x + 5, y + 2, 16, sf::Color::White);
}

void GameInterface::handleEvents(const sf::Event &event) {
    if (event.is<sf::Event::KeyPressed>()) {
        if (const auto *keyEvent = event.getIf<sf::Event::KeyPressed>()) {
            if (gameOver) {
                // Redémarrer si 'R' est pressé
                if (keyEvent->code == sf::Keyboard::Key::R) {
                    reset();
                } else if (keyEvent->code == sf::Keyboard::Key::Q) {
                    // Retourner au menu si 'Q' est pressé
                    inputHandler->handleKeyPressed(sf::Keyboard::Key::Q);
                }
                return;
            }

            // Traiter les entrées via le gestionnaire d'entrées GameInputHandler
            inputHandler->handleKeyPressed(keyEvent->code);
        }
    }
}

// Nouvelle méthode pour obtenir la couleur SFML à partir d'un code couleur
sf::Color GameInterface::getColorFromCode(int colorCode) {
    // Conversion des codes couleur en couleurs SFML
    switch (colorCode) {
    case 0:
        return sf::Color(0, 240, 240); // Cyan (CYAN)
    case 1:
        return sf::Color(240, 240, 0); // Jaune (YELLOW)
    case 2:
        return sf::Color(240, 0, 240); // Magenta (MAGENTA)
    case 3:
        return sf::Color(0, 240, 0); // Vert (GREEN)
    case 4:
        return sf::Color(240, 0, 0); // Rouge (RED)
    case 5:
        return sf::Color(0, 0, 240); // Bleu (BLUE)
    case 6:
        return sf::Color(240, 240, 240); // Blanc (WHITE)
    case 7:
        return sf::Color(120, 120, 120); // Gris (GRAY)
    case 8:
        return sf::Color(20, 20, 20); // Noir (BLACK)
    case 9:
    default:
        return sf::Color(50, 50, 50); // Gris foncé pour les cases vides (EMPTY)
    }
}

sf::Color GameInterface::getTileColor(tetris::TileColor color) {
    // Conversion des couleurs des tetrominos en couleurs SFML
    switch (color) {
    case tetris::TileColor::CYAN:
        return sf::Color(0, 240, 240); // Cyan
    case tetris::TileColor::YELLOW:
        return sf::Color(240, 240, 0); // Jaune
    case tetris::TileColor::MAGENTA:
        return sf::Color(240, 0, 240); // Magenta
    case tetris::TileColor::GREEN:
        return sf::Color(0, 240, 0); // Vert
    case tetris::TileColor::RED:
        return sf::Color(240, 0, 0); // Rouge
    case tetris::TileColor::BLUE:
        return sf::Color(0, 0, 240); // Bleu
    case tetris::TileColor::WHITE:
        return sf::Color(240, 240, 240); // Blanc
    case tetris::TileColor::BLACK:
        return sf::Color(20, 20, 20); // Noir
    case tetris::TileColor::GRAY:
        return sf::Color(120, 120, 120); // Gris
    case tetris::TileColor::EMPTY:
    default:
        return sf::Color(50, 50, 50); // Gris foncé pour les cases vides
    }
}

} // namespace tetris::gui
