#include "ClassicGUI.hpp"
#include "../../../network/GlobalConnexion.hpp"
#include "../../engine/SFMLEngine.hpp"
#include <algorithm> // For std::sort
#include <fcntl.h>
#include <iostream>
#include <mutex> // Pour les verrous
#include <packets_include.hpp>
#include <poll.h>
#include <queue>
#include <set>
#include <unistd.h>

namespace tetris::gui {

// Constantes pour la configuration
constexpr int EMPTY_COLOR = 9;
constexpr int PING_INTERVAL_MS = 200; // Intervalle de ping
constexpr int NETWORK_SLEEP_MS = 3;   // Temps de sommeil du thread réseau

// Constantes pour le rendu
constexpr float MAIN_WIDTH_RATIO = 0.45f;  // Ratio de la largeur principale
constexpr float MAIN_HEIGHT_RATIO = 0.75f; // Ratio de la hauteur principale
constexpr int MAIN_GRID_X = 30;            // Position X de la grille principale
constexpr int MAIN_GRID_Y = 50;            // Position Y de la grille principale
constexpr int PLAYER_COUNT_X_OFFSET = 150; // Décalage X du compteur de joueurs
constexpr int RIGHT_SIDE_PADDING = 80;     // Padding du côté droit
constexpr int PLAYERS_LABEL_SPACING = 20;  // Espacement des étiquettes de joueur
constexpr int WAITING_TEXT_Y_OFFSET = 100; // Décalage Y du texte d'attente
constexpr int CONTROLS_Y_OFFSET = 10;      // Décalage Y des contrôles
constexpr int CONTROLS_Y_SPACING = 20;     // Espacement vertical des contrôles

ClassicGUI::ClassicGUI(tetris::gui::SFMLEngine &engine, sf::RenderWindow *window)
    : gameInterface(engine, window), window(window), running(false), gameOver(false),
      playerCount(0), isOwner(false), engine(engine) {
    auto dummyGame = std::make_shared<tetris::TetrisGame>();
    inputHandler = std::make_unique<GameInputHandler>(dummyGame);
    inputHandler->setNetworkMode(true);
    pressedKeys.clear();

    // Pré-allocation de la grille pour éviter les réallocations en jeu
    gridData.resize(GRID_HEIGHT);
    for (auto &row : gridData) {
        row.resize(GRID_WIDTH, EMPTY_COLOR);
    }
}

ClassicGUI::~ClassicGUI() {
    stop();
}

void ClassicGUI::start() {
    gameOver = false;
    score = 0;
    running = true;
    otherPlayersGrids.clear();
    otherPlayersStatus.clear();
    pressedKeys.clear();

    try {
        inputHandler->returnToMenu = false;

        roomName = tetris::GlobalConnection::getInstance().get_current_room();
        playerCount = tetris::GlobalConnection::getInstance().get_player_count();
        isOwner = tetris::GlobalConnection::getInstance().is_room_owner();

        playerStates.clear();

        // Pré-allocation des états des joueurs
        for (int i = 0; i <= MAX_PLAYERS + 1; i++) {
            playerStates[i] = PlayerGameState();
            // Initialize grid with proper dimensions
            playerStates[i].grid.resize(GRID_HEIGHT);
            for (auto &row : playerStates[i].grid) {
                row.resize(GRID_WIDTH, EMPTY_COLOR);
            }
        }

        // Start network thread
        // update() is the sole active reader for this view.

    } catch (const std::exception &e) {
        running = false;
        gameOver = true;
    }
}

void ClassicGUI::stop() {
    if (!running.exchange(false))
        return;
    try {
        tetris::GlobalConnection::getInstance().send_input('q');
    } catch (const std::exception &) {
    }
}

void ClassicGUI::update(float deltaTime) {
    networkLoop();
    if (gameOver)
        return;

    if (inputHandler->returnToMenu) {
        stop();
    }
}

// Boucle de traitement réseau
void ClassicGUI::networkLoop() {
    if (!running)
        return;
    auto &connection = tetris::GlobalConnection::getInstance();
    for (int i = 0; i < 64 && running; ++i) {
        auto packet = connection.recepteur();
        if (!packet)
            break;
        processPacket(packet);
    }
    if (!connection.isConnected()) {
        gameOver = true;
        running = false;
    }
}

void ClassicGUI::processPacket(std::shared_ptr<tetris::Packet> packet) {
    if (!packet)
        return;

    if (auto *gridPacket = dynamic_cast<tetris::clientbound_packets::GridData *>(packet.get())) {
        if (!gridPacket->grid.empty()) {
            updatePlayerState(gridPacket->player, gridPacket->grid);
        }
    } else if (auto *nxtTilePacket =
                   dynamic_cast<tetris::clientbound_packets::NextTile *>(packet.get())) {
        refresh_next_piece(nxtTilePacket->next_piece);
    } else if (auto *stats =
                   dynamic_cast<tetris::clientbound_packets::ScorePacket *>(packet.get())) {
        score = stats->score;
    } else if (auto *gameEndPacket =
                   dynamic_cast<tetris::clientbound_packets::GameEnd *>(packet.get())) {
        std::lock_guard<std::mutex> lock(game_state_mutex);
        gameOver = true;
        running = false;
    } else if (auto *roomInfoPacket =
                   dynamic_cast<tetris::clientbound_packets::RoomInfos *>(packet.get())) {
        if (roomInfoPacket) {
            std::lock_guard<std::mutex> lock(player_mutex);
            roomName = roomInfoPacket->room_name;
            playerCount = roomInfoPacket->numb_player;

            // Pré-allocation des états des joueurs si nécessaire
            for (int i = 0; i <= MAX_PLAYERS + 1; i++) {
                if (playerStates.find(i) == playerStates.end()) {
                    playerStates[i] = PlayerGameState();
                    // Initialize grid with proper dimensions
                    playerStates[i].grid.resize(GRID_HEIGHT);
                    for (auto &row : playerStates[i].grid) {
                        row.resize(GRID_WIDTH, EMPTY_COLOR);
                    }
                }
            }
        }
    }
}

// Mise à jour de l'état d'un joueur
void ClassicGUI::updatePlayerState(int playerIndex, const std::vector<std::vector<int>> &newGrid) {
    if (newGrid.empty() || playerIndex < 0 || playerIndex > MAX_PLAYERS + 1)
        return;

    if (playerIndex == 0) {
        std::lock_guard<std::mutex> lock(grid_mutex);
        gridData = newGrid;
    } else {
        std::lock_guard<std::mutex> lock(player_mutex);
        playerStates[playerIndex].grid = newGrid;
        playerStates[playerIndex].active = true;
        otherPlayersGrids[playerIndex] = newGrid;
        otherPlayersStatus[playerIndex] = true;
    }
}

void ClassicGUI::refresh_next_piece(tetris::PieceType newPiece) {
    std::lock_guard<std::mutex> lock(game_state_mutex);
    nextPiece = newPiece;
}

// Vérification si une grille est vide (optimisée pour s'arrêter dès qu'une cellule non vide est
// trouvée)
bool ClassicGUI::allCellsEmpty(const std::vector<std::vector<int>> &grid) {
    if (grid.empty())
        return true;

    for (const auto &row : grid) {
        for (int cell : row) {
            if (cell != EMPTY_COLOR) {
                return false; // Retourne immédiatement si une cellule non vide est trouvée
            }
        }
    }
    return true;
}

// Gestion des événements clavier
void ClassicGUI::handleEvents(const sf::Event &event) {
    if (event.is<sf::Event::KeyPressed>()) {
        if (const auto *keyEvent = event.getIf<sf::Event::KeyPressed>()) {
            if (gameOver) {
                if (keyEvent->code == sf::Keyboard::Key::Q) {
                    running = false;
                    stop();
                }
                return;
            }

            // Traitement des nouvelles touches uniquement (anti-répétition)
            if (pressedKeys.find(keyEvent->code) == pressedKeys.end()) {
                pressedKeys.insert(keyEvent->code);
                inputHandler->handleKeyPressed(keyEvent->code);
            }
        }
    } else if (event.is<sf::Event::KeyReleased>()) {
        if (const auto *keyEvent = event.getIf<sf::Event::KeyReleased>()) {
            pressedKeys.erase(keyEvent->code);
        }
    }
}

void ClassicGUI::render() {
    // Copie rapide des données sous verrous pour minimiser la durée des blocages
    std::vector<std::vector<int>> mainGrid;
    tetris::PieceType currentNextPiece;
    bool isGameOver;
    std::string currentRoomName;

    int currentPlayerCount;

    {
        std::lock_guard<std::mutex> gridLock(grid_mutex);
        mainGrid = gridData;
    }

    {
        std::lock_guard<std::mutex> stateLock(game_state_mutex);
        currentNextPiece = nextPiece;
        isGameOver = gameOver;
        currentRoomName = roomName;
        currentPlayerCount = playerCount;
    }

    // Rendu sans verrous
    window->clear(sf::Color(20, 20, 20));

    // Affichage du nombre de joueurs
    engine.drawText("Joueurs: " + std::to_string(currentPlayerCount),
                    window->getSize().x - PLAYER_COUNT_X_OFFSET, 10, 18, sf::Color::White);

    // Affichage du nom de la salle
    if (!currentRoomName.empty()) {
        engine.drawText("Salle: " + currentRoomName, 25, 10, 18, sf::Color::White);
    }

    // Calculs de mise en page
    const float windowWidth = static_cast<float>(window->getSize().x);
    const float windowHeight = static_cast<float>(window->getSize().y);
    const float mainWidth = windowWidth * MAIN_WIDTH_RATIO;
    const float mainHeight = windowHeight * MAIN_HEIGHT_RATIO;

    // Grille du joueur principal
    gameInterface.renderWithNetworkData(mainGrid, score, currentNextPiece, isGameOver, MAIN_GRID_X,
                                        MAIN_GRID_Y, static_cast<int>(mainWidth),
                                        static_cast<int>(mainHeight));

    // Contrôles - utilisation des constantes pour les espacements
    const int controlsBaseY = MAIN_GRID_Y + static_cast<int>(mainHeight);
    engine.drawText("CONTROLES : ", MAIN_GRID_X, controlsBaseY + CONTROLS_Y_OFFSET, 16,
                    sf::Color::White);
    engine.drawText("GAUCHE/DROITE: Deplacer - HAUT: Rotation - BAS: Descendre", MAIN_GRID_X,
                    controlsBaseY + CONTROLS_Y_OFFSET + CONTROLS_Y_SPACING + 5, 14,
                    sf::Color::White);
    engine.drawText("Q: Quitter", MAIN_GRID_X,
                    controlsBaseY + CONTROLS_Y_OFFSET + (CONTROLS_Y_SPACING + 5) * 2, 16,
                    sf::Color::Red);

    // Affichage des autres joueurs
    renderOtherPlayers(MAIN_GRID_X, MAIN_GRID_Y, mainWidth, mainHeight, windowWidth, windowHeight);
}

// Rendu des grilles des autres joueurs
void ClassicGUI::renderOtherPlayers(int mainGridX, int mainGridY, float mainWidth, float mainHeight,
                                    float windowWidth, float windowHeight) {
    const float rightSideWidth = windowWidth - mainWidth - RIGHT_SIDE_PADDING;
    const float rightSideX = mainGridX + mainWidth + PLAYERS_LABEL_SPACING;

    // Récupération des joueurs actifs avec verrouillage approprié
    std::vector<int> activePlayerIds;
    std::map<int, PlayerGameState> statesCopy;

    {
        std::lock_guard<std::mutex> playerLock(player_mutex);
        statesCopy = playerStates;
    }

    activePlayerIds.reserve(MAX_PLAYERS);

    for (const auto &[playerId, state] : statesCopy) {
        if (playerId > 0 && playerId <= MAX_PLAYERS + 1 && state.active && !allCellsEmpty(state.grid)) {
            activePlayerIds.push_back(playerId);
        }
    }

    std::sort(activePlayerIds.begin(), activePlayerIds.end());

    if (!activePlayerIds.empty()) {
        engine.drawText("AUTRES JOUEURS", rightSideX, 10, 18, sf::Color::Yellow);

        const int activePlayers = activePlayerIds.size();
        const int gridsPerCol = (activePlayers > 2) ? 2 : 1;
        const int gridsPerRow = (activePlayers + gridsPerCol - 1) / gridsPerCol;

        const float otherGridWidth = rightSideWidth / gridsPerRow * 0.9f;
        const float otherGridHeight = windowHeight * 0.35f;
        const float horizontalMargin = rightSideWidth * 0.1f / (gridsPerRow + 1);
        const float verticalMargin = PLAYERS_LABEL_SPACING;

        for (int i = 0; i < activePlayerIds.size(); i++) {
            const int playerId = activePlayerIds[i];

            // Calcul de la position de la grille
            const int row = i / gridsPerRow;
            const int col = i % gridsPerRow;
            const float gridX =
                rightSideX + col * (otherGridWidth + horizontalMargin) + horizontalMargin;
            const float gridY = mainGridY + row * (otherGridHeight + verticalMargin);

            // Étiquette et couleur du joueur
            const std::string playerLabel = "Joueur " + std::to_string(playerId);
            const sf::Color playerColor = sf::Color(100 + playerId * 40, 140, 100 + playerId * 40);

            engine.drawText(playerLabel, gridX, gridY - PLAYERS_LABEL_SPACING, 14, playerColor);
            gameInterface.drawNetworkGrid(statesCopy.at(playerId).grid, static_cast<int>(gridX),
                                          static_cast<int>(gridY), static_cast<int>(otherGridWidth),
                                          static_cast<int>(otherGridHeight));
        }
    } else {
        // Message d'attente
        engine.drawText("En attente d'autres joueurs...", rightSideX,
                        mainGridY + WAITING_TEXT_Y_OFFSET, 18, sf::Color(128, 128, 128));
    }
}

bool ClassicGUI::wantsToReturnToMenu() const {
    return !running;
}

void ClassicGUI::handlePlayerElimination(int playerIndex) {
    std::lock_guard<std::mutex> lock(player_mutex);
    if (playerStates.find(playerIndex) != playerStates.end()) {
        playerStates[playerIndex].active = false;
    }
    otherPlayersStatus[playerIndex] = false;
}

} // namespace tetris::gui