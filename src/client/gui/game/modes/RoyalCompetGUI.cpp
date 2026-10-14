#include "RoyalCompetGUI.hpp"
#include "../../../network/GlobalConnexion.hpp"
#include "../../engine/SFMLEngine.hpp"
#include <algorithm> // For std::sort
#include <fcntl.h>
#include <game/network_board.hpp>
#include <iostream>
#include <mutex> // Pour les verrous
#include <packets_include.hpp>
#include <poll.h>
#include <queue>
#include <serverbound_packets/malus_data.hpp>
#include <set>
#include <unistd.h>

namespace tetris::gui {

// Constantes pour la configuration
constexpr int EMPTY_COLOR = 8;
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

RoyalCompetitionGUI::RoyalCompetitionGUI(tetris::gui::SFMLEngine &engine, sf::RenderWindow *window)
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

    // Initialiser le menu des malus
    initMalusMenu();
}

void RoyalCompetitionGUI::initMalusMenu() {
    // Initialiser la liste des malus disponibles avec leurs coûts
    availableMaluses = {{"Inversion commandes", 3, MalusType::InvertControls},
                        {"Blocage commandes", 2, MalusType::BlockControls},
                        {"Accelerer chute", 2, MalusType::SpeedFall},
                        {"Nettoyer 4 lignes", 3, MalusType::Lightning2x2},
                        {"Extinction ecran", 4, MalusType::ScreenBlackout},
                        {"Blocs 1x1", 2, MalusType::Blocks1x1}};
}

RoyalCompetitionGUI::~RoyalCompetitionGUI() {
    stop();
}

void RoyalCompetitionGUI::start() {
    energy = 0;
    gameOver = false;
    running = true;
    otherPlayersGrids.clear();
    otherPlayersStatus.clear();
    pressedKeys.clear();
    malusMenuOpen = false;
    targetedPlayerId = -1;
    malusSelections.clear();

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

void RoyalCompetitionGUI::stop() {
    if (!running.exchange(false))
        return;
    try {
        tetris::GlobalConnection::getInstance().send_input('q');
    } catch (const std::exception &) {
    }
}

void RoyalCompetitionGUI::update(float deltaTime) {
    networkLoop();
    if (gameOver)
        return;

    if (inputHandler->returnToMenu) {
        stop();
    }
}

// Boucle de traitement réseau
void RoyalCompetitionGUI::networkLoop() {
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

void RoyalCompetitionGUI::processPacket(std::shared_ptr<tetris::Packet> packet) {
    if (!packet)
        return;

    if (auto *gridPacket = dynamic_cast<tetris::clientbound_packets::GridData *>(packet.get())) {
        if (!gridPacket->grid.empty()) {
            // Vérifier si le joueur local vient d'être éliminé
            if (gridPacket->player == 0) {
                bool wasEliminated = false;
                {
                    std::lock_guard<std::mutex> lock(grid_mutex);
                    wasEliminated = !gridData.empty() && isPlayerEliminated(gridData);
                }

                updatePlayerState(gridPacket->player, gridPacket->grid);

                // Vérifier si le joueur est maintenant éliminé
                bool isNowEliminated = isPlayerEliminated(gridPacket->grid);

                // Si le joueur vient d'être éliminé, mettre son énergie à 0
                if (!wasEliminated && isNowEliminated) {
                    std::lock_guard<std::mutex> lock(game_state_mutex);
                    energy = 0;
                }
            } else {
                updatePlayerState(gridPacket->player, gridPacket->grid);
            }
        }
    } else if (auto *scorePacket =
                   dynamic_cast<tetris::clientbound_packets::ScorePacket *>(packet.get())) {
        std::lock_guard<std::mutex> lock(game_state_mutex);

        // Ne mettre à jour l'énergie que si le joueur n'est pas éliminé
        bool playerEliminated = false;
        {
            std::lock_guard<std::mutex> gridLock(grid_mutex);
            playerEliminated = !gridData.empty() && isPlayerEliminated(gridData);
        }

        if (!playerEliminated) {
            energy = scorePacket->energy;
        }
    } else if (auto *malusPacket =
                   dynamic_cast<tetris::clientbound_packets::Malus *>(packet.get())) {
        std::lock_guard<std::mutex> lock(game_state_mutex);
        malus = malusPacket->malus;
    } else if (auto *nextTilePacket =
                   dynamic_cast<tetris::clientbound_packets::NextTile *>(packet.get())) {
        std::lock_guard<std::mutex> lock(game_state_mutex);
        nextPiece = nextTilePacket->next_piece;
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
void RoyalCompetitionGUI::updatePlayerState(int playerIndex,
                                            const std::vector<std::vector<int>> &newGrid) {
    if (newGrid.empty() || playerIndex < 0 || playerIndex > MAX_PLAYERS + 1)
        return;

    if (playerIndex == 0) {
        std::lock_guard<std::mutex> lock(grid_mutex);
        gridData = newGrid;
    } else {
        std::lock_guard<std::mutex> lock(player_mutex);
        playerStates[playerIndex].grid = newGrid;
        playerStates[playerIndex].active = true;

        // Vérifier si le joueur est éliminé (grille rouge)
        playerStates[playerIndex].eliminated = isPlayerEliminated(newGrid);

        otherPlayersGrids[playerIndex] = newGrid;
        otherPlayersStatus[playerIndex] = true;
    }
}

// Vérification si une grille est vide (optimisée pour s'arrêter dès qu'une cellule non vide est
// trouvée)
bool RoyalCompetitionGUI::allCellsEmpty(const std::vector<std::vector<int>> &grid) {
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

bool RoyalCompetitionGUI::isPlayerEliminated(const std::vector<std::vector<int>> &grid) {
    return tetris::isEliminatedNetworkBoard(grid);
}

// Vérification si le joueur a assez d'énergie pour un malus
bool RoyalCompetitionGUI::hasEnoughEnergy(int malusCost) const {
    std::lock_guard<std::mutex> lock(game_state_mutex);
    return energy >= malusCost;
}

// Conversion de notre type de malus en type serveur
tetris::Malus RoyalCompetitionGUI::convertMalusTypeToServerEnum(MalusType type) {
    switch (type) {
    case MalusType::InvertControls:
        return tetris::Malus::INVERT_CONTROLS;
    case MalusType::BlockControls:
        return tetris::Malus::BLOCK_CONTROLS;
    case MalusType::SpeedFall:
        return tetris::Malus::SPEED_UP;
    case MalusType::Lightning2x2:
        return tetris::Malus::STRIKE;
    case MalusType::ScreenBlackout:
        return tetris::Malus::SHUT_DOWN;
    case MalusType::Blocks1x1:
        return tetris::Malus::ONE_BY_ONE_BLOCK;
    default:
        return tetris::Malus::NO;
    }
}

// Envoi d'un malus au serveur
void RoyalCompetitionGUI::sendMalusToServer(int targetPlayerId, MalusType malusType) {
    try {
        // Convertir notre type de malus en type serveur
        tetris::Malus serverMalus = convertMalusTypeToServerEnum(malusType);

        // Créer et envoyer le paquet de malus
        tetris::GlobalConnection::getInstance().send_malus(serverMalus, targetPlayerId);

        // Réduire l'énergie du joueur en fonction du coût du malus
        int malusCost = 0;
        for (const auto &malus : availableMaluses) {
            if (malus.type == malusType) {
                malusCost = malus.cost;
                break;
            }
        }

        // Mettre à jour l'énergie localement
        std::lock_guard<std::mutex> lock(game_state_mutex);
        energy = std::max(0, energy - malusCost);

    } catch (const std::exception &e) {
        std::cerr << "Erreur lors de l'envoi du malus: " << e.what() << std::endl;
    }
}

// Stockage d'une sélection de malus
void RoyalCompetitionGUI::storeMalusSelection(int targetPlayerId, MalusType malusType) {
    // Trouver le coût du malus sélectionné
    int malusCost = 0;
    for (const auto &malus : availableMaluses) {
        if (malus.type == malusType) {
            malusCost = malus.cost;
            break;
        }
    }

    // La vérification de l'énergie est maintenant faite avant l'appel à cette méthode
    // Stocker la sélection de malus
    {
        std::lock_guard<std::mutex> lock(malus_mutex);
        malusSelections.push_back({0, targetPlayerId, malusType}); // 0 est l'ID du joueur local
    }

    // Envoyer le malus au serveur
    sendMalusToServer(targetPlayerId, malusType);

    // Fermer le menu des malus
    malusMenuOpen = false;
    targetedPlayerId = -1;
}

// Vérification si une grille de joueur a été cliquée
bool RoyalCompetitionGUI::checkPlayerGridClicked(int playerId, float x, float y, float width,
                                                 float height) {
    sf::Vector2i mousePos = sf::Mouse::getPosition(*window);
    return mousePos.x >= x && mousePos.x <= x + width && mousePos.y >= y &&
           mousePos.y <= y + height;
}

// Gestion des événements clavier et souris
void RoyalCompetitionGUI::handleEvents(const sf::Event &event) {
    if (event.is<sf::Event::KeyPressed>()) {
        if (const auto *keyEvent = event.getIf<sf::Event::KeyPressed>()) {
            if (gameOver) {
                if (keyEvent->code == sf::Keyboard::Key::Q) {
                    running = false;
                    stop();
                }
                return;
            }

            // Si le menu des malus est ouvert, on peut le fermer avec Escape
            if (malusMenuOpen && keyEvent->code == sf::Keyboard::Key::Escape) {
                malusMenuOpen = false;
                targetedPlayerId = -1;
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
    } else if (event.is<sf::Event::MouseButtonPressed>()) {
        if (const auto *mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mouseEvent->button == sf::Mouse::Button::Left) {
                // Si le menu des malus est ouvert, vérifier si un malus a été sélectionné
                if (malusMenuOpen && targetedPlayerId >= 0) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(*window);
                    for (size_t i = 0; i < availableMaluses.size(); i++) {
                        const auto &malus = availableMaluses[i];
                        float itemY = malusMenuPosition.y + MALUS_MENU_PADDING + 50 +
                                      i * MALUS_MENU_ITEM_HEIGHT;

                        // Vérifier si le clic est sur cet élément du menu
                        if (mousePos.x >= malusMenuPosition.x + MALUS_MENU_PADDING &&
                            mousePos.x <=
                                malusMenuPosition.x + MALUS_MENU_WIDTH - MALUS_MENU_PADDING &&
                            mousePos.y >= itemY && mousePos.y <= itemY + MALUS_MENU_ITEM_HEIGHT) {

                            // Vérifier si le joueur a assez d'énergie pour ce malus
                            if (hasEnoughEnergy(malus.cost)) {
                                // Un malus a été sélectionné et le joueur a assez d'énergie
                                storeMalusSelection(targetedPlayerId, malus.type);
                            }
                            return;
                        }
                    }

                    // Clic en dehors du menu, on le ferme
                    if (mousePos.x < malusMenuPosition.x ||
                        mousePos.x > malusMenuPosition.x + MALUS_MENU_WIDTH ||
                        mousePos.y < malusMenuPosition.y ||
                        mousePos.y > malusMenuPosition.y + MALUS_MENU_HEIGHT) {
                        malusMenuOpen = false;
                        targetedPlayerId = -1;
                    }
                }
                // Sinon, vérifier si un joueur a été cliqué
                else {
                    // Vérifier d'abord si le joueur local n'est pas éliminé
                    bool localPlayerEliminated = false;
                    {
                        std::lock_guard<std::mutex> lock(grid_mutex);
                        localPlayerEliminated = !gridData.empty() && isPlayerEliminated(gridData);
                    }

                    // Si le joueur local est éliminé, il ne peut plus envoyer de malus
                    if (localPlayerEliminated) {
                        return;
                    }

                    // Calculer les dimensions de la fenêtre pour le positionnement des grilles
                    const float windowWidth = static_cast<float>(window->getSize().x);
                    const float windowHeight = static_cast<float>(window->getSize().y);
                    const float mainWidth = windowWidth * MAIN_WIDTH_RATIO;
                    const float mainHeight = windowHeight * MAIN_HEIGHT_RATIO;
                    const float rightSideWidth = windowWidth - mainWidth - RIGHT_SIDE_PADDING;
                    const float rightSideX = MAIN_GRID_X + mainWidth + PLAYERS_LABEL_SPACING;

                    // Récupération des joueurs actifs
                    std::vector<int> activePlayerIds;
                    std::map<int, PlayerGameState> statesCopy;

                    {
                        std::lock_guard<std::mutex> playerLock(player_mutex);
                        statesCopy = playerStates;
                    }

                    activePlayerIds.reserve(MAX_PLAYERS);

                    for (const auto &[playerId, state] : statesCopy) {
                        if (playerId > 0 && playerId <= MAX_PLAYERS + 1 && state.active &&
                            !allCellsEmpty(state.grid)) {
                            activePlayerIds.push_back(playerId);
                        }
                    }

                    std::sort(activePlayerIds.begin(), activePlayerIds.end());

                    if (!activePlayerIds.empty()) {
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
                            const float gridX = rightSideX +
                                                col * (otherGridWidth + horizontalMargin) +
                                                horizontalMargin;
                            const float gridY =
                                MAIN_GRID_Y + row * (otherGridHeight + verticalMargin);

                            // Vérifier si la grille a été cliquée
                            if (checkPlayerGridClicked(playerId, gridX, gridY, otherGridWidth,
                                                       otherGridHeight)) {
                                // Vérifier si le joueur n'est pas éliminé avant d'ouvrir le menu
                                if (!playerStates[playerId].eliminated) {
                                    malusMenuOpen = true;
                                    targetedPlayerId = playerId;
                                    // Positionner le menu près de la grille cliquée
                                    malusMenuPosition.x = gridX + otherGridWidth / 2;
                                    malusMenuPosition.y = gridY;
                                    // Ajuster la position pour éviter que le menu sorte de l'écran
                                    if (malusMenuPosition.x + MALUS_MENU_WIDTH > windowWidth) {
                                        malusMenuPosition.x = windowWidth - MALUS_MENU_WIDTH;
                                    }
                                    if (malusMenuPosition.y + MALUS_MENU_HEIGHT > windowHeight) {
                                        malusMenuPosition.y = windowHeight - MALUS_MENU_HEIGHT;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

void RoyalCompetitionGUI::render() {
    // Copie rapide des données sous verrous pour minimiser la durée des blocages
    std::vector<std::vector<int>> mainGrid;
    int currentEnergy;
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
        currentEnergy = energy;
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
    gameInterface.renderWithNetworkData(mainGrid, currentEnergy, currentNextPiece, isGameOver,
                                        MAIN_GRID_X, MAIN_GRID_Y, static_cast<int>(mainWidth),
                                        static_cast<int>(mainHeight), "ENERGY");

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

    // Affichage du menu des malus si ouvert
    if (malusMenuOpen && targetedPlayerId >= 0) {
        renderMalusMenu();
    }
}

// Rendu du menu des malus
void RoyalCompetitionGUI::renderMalusMenu() {
    // Arrière-plan du menu
    engine.drawRectangle(malusMenuPosition.x, malusMenuPosition.y, MALUS_MENU_WIDTH,
                         MALUS_MENU_HEIGHT + 10, sf::Color(20, 20, 20, 230));

    // Bordure du menu
    engine.drawRectangle(malusMenuPosition.x, malusMenuPosition.y, MALUS_MENU_WIDTH,
                         MALUS_MENU_HEIGHT, sf::Color(60, 60, 60, 180) // Bordure
    );

    // Titre du menu
    engine.drawText("Malus - Joueur " + std::to_string(targetedPlayerId),
                    malusMenuPosition.x + MALUS_MENU_PADDING,
                    malusMenuPosition.y + MALUS_MENU_PADDING, 18, sf::Color::Yellow);

    // Énergie disponible
    int currentEnergy;
    {
        std::lock_guard<std::mutex> lock(game_state_mutex);
        currentEnergy = energy;
    }

    engine.drawText("Energie: " + std::to_string(currentEnergy),
                    malusMenuPosition.x + MALUS_MENU_PADDING,
                    malusMenuPosition.y + MALUS_MENU_PADDING + 20, 16, sf::Color::White);

    // Liste des malus disponibles
    for (size_t i = 0; i < availableMaluses.size(); i++) {
        const auto &malus = availableMaluses[i];
        float itemY = malusMenuPosition.y + MALUS_MENU_PADDING + 50 + i * MALUS_MENU_ITEM_HEIGHT;

        // Vérifier si le joueur a assez d'énergie pour ce malus
        bool isAvailable = hasEnoughEnergy(malus.cost);

        // Couleur du texte (gris si pas assez d'énergie, blanc sinon)
        sf::Color textColor = isAvailable ? sf::Color::White : sf::Color(100, 100, 100);

        // Arrière-plan de l'élément
        sf::Color bgColor = isAvailable ? sf::Color(40, 40, 40, 200) : sf::Color(25, 25, 25, 200);

        // Afficher l'arrière-plan de l'élément
        engine.drawRectangle(malusMenuPosition.x + MALUS_MENU_PADDING, itemY,
                             MALUS_MENU_WIDTH - 2 * MALUS_MENU_PADDING, MALUS_MENU_ITEM_HEIGHT - 2,
                             bgColor);

        // Si l'option n'est pas disponible, afficher un effet "désactivé"
        if (!isAvailable) {
            // Ligne diagonale ou effet visuel indiquant que l'item est désactivé
            engine.drawRectangle(malusMenuPosition.x + MALUS_MENU_PADDING, itemY,
                                 MALUS_MENU_WIDTH - 2 * MALUS_MENU_PADDING, 1,
                                 sf::Color(60, 60, 60, 150));

            engine.drawRectangle(
                malusMenuPosition.x + MALUS_MENU_PADDING, itemY + MALUS_MENU_ITEM_HEIGHT - 3,
                MALUS_MENU_WIDTH - 2 * MALUS_MENU_PADDING, 1, sf::Color(60, 60, 60, 150));
        }

        // Afficher le nom du malus
        engine.drawText(malus.name, malusMenuPosition.x + 2 * MALUS_MENU_PADDING, itemY + 5, 14,
                        textColor);

        // Afficher le coût du malus
        engine.drawText(std::to_string(malus.cost),
                        malusMenuPosition.x + MALUS_MENU_WIDTH - 3 * MALUS_MENU_PADDING, itemY + 5,
                        14, textColor);
    }
}

// Rendu des grilles des autres joueurs
void RoyalCompetitionGUI::renderOtherPlayers(int mainGridX, int mainGridY, float mainWidth,
                                             float mainHeight, float windowWidth,
                                             float windowHeight) {
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
        if (playerId > 0 && playerId <= MAX_PLAYERS + 1 && state.active &&
            !allCellsEmpty(state.grid)) {
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
            sf::Color playerColor = sf::Color(100 + playerId * 40, 140, 100 + playerId * 40);

            // Si c'est le joueur ciblé, changer la couleur de l'étiquette
            if (playerId == targetedPlayerId) {
                playerColor = sf::Color::Yellow;
            }
            // Si le joueur est éliminé, afficher en gris
            else if (playerStates[playerId].eliminated) {
                playerColor = sf::Color(100, 100, 100); // Gris
            }

            // Ajuster le texte de l'étiquette pour les joueurs éliminés
            std::string displayLabel = playerLabel;
            if (playerStates[playerId].eliminated) {
                displayLabel += " (elimine)";
            }

            engine.drawText(displayLabel, gridX, gridY - PLAYERS_LABEL_SPACING, 14, playerColor);
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

bool RoyalCompetitionGUI::wantsToReturnToMenu() const {
    return !running;
}

void RoyalCompetitionGUI::sendMalus(int lines) {
    // Implémentation future
}

void RoyalCompetitionGUI::receiveMalus(int lines) {
    // Implémentation future
}

void RoyalCompetitionGUI::handlePlayerElimination(int playerIndex) {
    std::lock_guard<std::mutex> lock(player_mutex);
    if (playerStates.find(playerIndex) != playerStates.end()) {
        playerStates[playerIndex].active = false;
    }
    otherPlayersStatus[playerIndex] = false;
}

void RoyalCompetitionGUI::refresh_grid(int playerIndex, const std::vector<std::vector<int>> &grid) {
    updatePlayerState(playerIndex, grid);
}

void RoyalCompetitionGUI::refresh_energy(int newEnergyLevel) {
    std::lock_guard<std::mutex> lock(game_state_mutex);
    energy = newEnergyLevel;
}

void RoyalCompetitionGUI::refresh_next_piece(tetris::PieceType newPiece) {
    std::lock_guard<std::mutex> lock(game_state_mutex);
    nextPiece = newPiece;
}

} // namespace tetris::gui