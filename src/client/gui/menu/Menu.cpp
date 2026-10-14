#include "Menu.hpp"
#include "../engine/InputHandler.hpp"
#include "../engine/SFMLEngine.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <fcntl.h>
#include <future> // For std::async
#include <iostream>
#include <poll.h>
#include <string>

// Déclaration de la fonction recepteur_avec_timeout
// std::shared_ptr<tetris::Packet> recepteur_avec_timeout(int socket, int timeout_ms);

const std::string ICON_CONNEXION = ASSETS_SRC "/menu/ConnexionIcon.png";
const std::string ICON_PASSWORD = ASSETS_SRC "/menu/PasswordIcon.png";
const std::string ICON_EYE_OPEN = ASSETS_SRC "/menu/eye_open.png";
const std::string ICON_EYE_CLOSED = ASSETS_SRC "/menu/eye_closed.png";
const std::string ICON_BACKGROUND = ASSETS_SRC "/menu/background.png";

Menu::Menu(tetris::gui::SFMLEngine &engine, sf::RenderWindow *window)
    : engine(engine), window(window), startGame(false),
      loginUsernameBox(window->getSize().x / 2 - 100, 250, 250, 50, "Nom d'utilisateur", false,
                       ASSETS_SRC "/menu/fond_blanc.png", 2),
      loginPasswordBox(window->getSize().x / 2 - 100, 320, 250, 50, "Mot de passe", true,
                       ASSETS_SRC "/menu/fond_blanc.png", 2),
      registerUsernameBox(window->getSize().x / 2 - 100, 250, 250, 50, "Nom d'utilisateur", false,
                          ASSETS_SRC "/menu/fond_blanc.png", 2),
      registerPasswordBox(window->getSize().x / 2 - 100, 320, 250, 50, "Mot de passe", true,
                          ASSETS_SRC "/menu/fond_blanc.png", 2),
      addFriendBox(window->getSize().x / 2 - 100, 250, 250, 50, "Nom d'utilisateur", false,
                   ASSETS_SRC "/menu/fond_blanc.png", 2),
      removeFriendBox(window->getSize().x / 2 - 100, 250, 250, 50, "Nom d'utilisateur", false,
                      ASSETS_SRC "/menu/fond_blanc.png", 2),
      createRoomBox(window->getSize().x / 2 - 100, 250, 250, 50, "Nom de la room", false,
                    ASSETS_SRC "/menu/fond_blanc.png", 2),
      joinRoomBox(window->getSize().x / 2 - 100, 250, 250, 50, "Nom de la room", false,
                  ASSETS_SRC "/menu/fond_blanc.png", 2),
      sendMessageToBox(100, 200, 300, 40, "Destinataire", false, ASSETS_SRC "/menu/fond_blanc.png",
                       2),
      sendMessageContentBox(100, 270, 300, 40, "Message", false, ASSETS_SRC "/menu/fond_blanc.png",
                            2),
      roomChatBox(window->getSize().x / 2 - 100, 250, 250, 50, "Nom de la room", false,
                  ASSETS_SRC "/menu/fond_blanc.png", 2)

{
    engine.loadBackground(ASSETS_SRC "/menu/background.png", window->getSize().x,
                          window->getSize().y);

    // Configurer le callback de mise à jour de la room
    room.setOnRoomUpdateCallback([this]() {
        // Update using the room's current state
        current_room = room.getRoomName();
        player_in_room = room.getPlayerCount();
        mode = room.getGameMode();
    });
}

Menu::~Menu() {}

void Menu::update(float deltaTime) {
    room.update();
    // Mise à jour des champs de saisie
    if (currentState == MenuState::LoginInput) {
        loginUsernameBox.update();
        if (loginUsernameBox.getText().size() > 15) {
            loginUsernameBox.setText(loginUsernameBox.getText().substr(0, 15));
        }
        loginPasswordBox.update();
    }

    if (currentState == MenuState::RegisterInput) {
        registerUsernameBox.update();
        if (registerUsernameBox.getText().size() > 15) {
            registerUsernameBox.setText(registerUsernameBox.getText().substr(0, 15));
        }
        registerPasswordBox.update();
    }

    // Vérifie si le jeu doit démarrer (signal du thread)
    if (room.shouldStartGame()) {

        try {

            // Lance le bon mode de jeu
            if (mode == tetris::GameMode::Endless) {
                endlessGame = std::make_unique<tetris::gui::EndlessGUI>(engine, window);
                endlessGame->start();
                currentState = MenuState::EndlessMode;
                // std::cout << "Endless game started" << std::endl;
            } else if (mode == tetris::GameMode::Classique) {
                classicGame = std::make_unique<tetris::gui::ClassicGUI>(engine, window);
                classicGame->start();
                currentState = MenuState::ClassicGameActive;
                // std::cout << "Classic game started, state changed to ClassicGameActive" <<
                // std::endl;
            } else if (mode == tetris::GameMode::Duel) {
                duelGame = std::make_unique<tetris::gui::DuelGUI>(engine, window);
                duelGame->start();
                currentState = MenuState::DuelModeActive;
                // std::cout << "Duel game started, state changed to DuelModeActive" << std::endl;
            } else if (mode == tetris::GameMode::BR) {
                royalCompetitionGame =
                    std::make_unique<tetris::gui::RoyalCompetitionGUI>(engine, window);
                royalCompetitionGame->start();
                currentState = MenuState::RoyalCompetitionActive;
                // std::cout << "Royal Competition game started, state changed to
                // RoyalCompetitionActive" << std::endl;
            }

            room.resetStartGameFlag();

        } catch (const std::exception &e) {
            std::cerr << "Exception while starting game: " << e.what() << std::endl;
            // start = false;

            room.resetStartGameFlag();
            currentState = MenuState::RoomWaiting;
        }
    }

    // Mise à jour des jeux en cours
    if (currentState == MenuState::EndlessMode && endlessGame) {
        endlessGame->update(deltaTime);

        if (endlessGame->wantsToReturnToMenu()) {
            endlessGame.reset();
            currentState = MenuState::SoloMode;
        }
    }

    if (currentState == MenuState::ClassicGameActive && classicGame) {
        classicGame->update(deltaTime);

        if (classicGame->wantsToReturnToMenu()) {
            classicGame.reset();
            room.leaveRoom();
            currentState = MenuState::MultiPlayerMode;
        }
    }

    if (currentState == MenuState::DuelModeActive && duelGame) {
        duelGame->update(deltaTime);

        if (duelGame->wantsToReturnToMenu()) {
            duelGame.reset();
            room.leaveRoom();
            currentState = MenuState::MultiPlayerMode;
        }
    }

    if (currentState == MenuState::RoyalCompetitionActive && royalCompetitionGame) {
        royalCompetitionGame->update(deltaTime);

        if (royalCompetitionGame->wantsToReturnToMenu()) {
            royalCompetitionGame.reset();
            room.leaveRoom();
            currentState = MenuState::MultiPlayerMode;
        }
    }
}

void Menu::render() {
    switch (currentState) {
    case MenuState::MainMenu:
        renderMainMenu();
        break;
    case MenuState::LoginInput:
        renderLoginInput();
        break;
    case MenuState::RegisterInput:
        renderRegisterInput();
        break;
    case MenuState::GameMenu:
        renderGameMenu();
        break;
    case MenuState::GameModeMenu:
        renderGameMode();
        break;
    case MenuState::SoloMode:
        renderSoloMode();
        break;
    case MenuState::MultiPlayerMode:
        renderMultiPlayerMode();
        break;
    case MenuState::RoomWaiting:
        room_waiting(mode);
        break;
    case MenuState::Discussion:
        renderDiscussion();
        break;
    case MenuState::CreateRoom:
        renderCreateRoom();
        break;
    case MenuState::CreateRoomResponse:
        renderCreateRoomResponse();
        break;
    case MenuState::JoinRoom:
        renderJoinRoom();
        break;
    case MenuState::JoinRoomResponse:
        renderJoinRoomResponse();
        break;
    case MenuState::HostStartGame:
        hostStartGame();
        break;
    case MenuState::FriendsMenu:
        renderFriendsMenu();
        break;
    case MenuState::AddFriends:
        renderAddFriends();
        break;
    case MenuState::RemoveFriends:
        renderRemoveFriends();
        break;
    case MenuState::ManageFriendsRequests:
        renderManageFriendsRequests();
        break;
    case MenuState::FriendsRequestsResponse:
        renderFriendsRequestsResponse();
        break;
    case MenuState::FriendsList:
        renderFriendList();
        break;
    case MenuState::TopScores:
        renderTopScores();
        break;
    case MenuState::QuitConfirmation:
        renderQuitConfirmation();
        break;
    case MenuState::QUIT:
        engine.getWindow()->close();
        break;
    case MenuState::EndlessMode:
        renderEndlessMode();
        break;
    case MenuState::ClassicGameActive:
        renderClassicGameActive();
        break;
    case MenuState::Messaging:
        renderMessaging();
        break;
    case MenuState::DuelModeActive:
        renderDuelModeActive();
        break;
    case MenuState::RoyalCompetitionActive:
        renderRoyalCompetitionActive();
        break;
    }
}

void Menu::renderButton(const std::string text, std::vector<std::string> buttonTexts) {

    int buttonWidth = 300;
    int buttonHeight = 50;
    int spacing = 20;
    int startY = 200;

    // Centrage horizontal
    int centerX = window->getSize().x / 2 - buttonWidth / 2;

    // Affichage du titre
    engine.drawText(text, window->getSize().x / 2 - 150, 50, 40, sf::Color::White);

    // Affichage des boutons
    for (int i = 0; i < buttonTexts.size(); ++i) {
        engine.drawRectangle(centerX, startY + i * (buttonHeight + spacing), buttonWidth,
                             buttonHeight, sf::Color::Blue);
        engine.drawText(buttonTexts[i], centerX + 35, startY + i * (buttonHeight + spacing) + 10,
                        24, sf::Color::White);
    }
}

void Menu::resetInputFields() {
    loginUsernameBox.setText("");
    loginPasswordBox.setText("");
    registerUsernameBox.setText("");
    registerPasswordBox.setText("");
    addFriendBox.setText("");
    removeFriendBox.setText("");
    createRoomBox.setText("");
    joinRoomBox.setText("");
    sendMessageToBox.setText("");
    sendMessageContentBox.setText("");
    roomChatBox.setText("");
}

void Menu::renderMainMenu() {
    engine.drawBackground();
    auto winSize = window->getSize();
    float buttonWidth = winSize.x * 0.3f;
    float buttonHeight = winSize.y * 0.08f;
    float spacing = winSize.y * 0.05f;
    float startY = winSize.y * 0.4f;
    float centerX = winSize.x / 2.f - buttonWidth / 2.f;

    // Titre centré
    int titleSize = static_cast<int>(winSize.y * 0.06f);
    engine.drawText("    Tetris Royale", winSize.x / 2.f - (titleSize * 4.f), winSize.y * 0.15f,
                    titleSize, sf::Color::White);

    std::vector<std::string> mainButtons = {"Connexion", "S'inscrire", "Quitter"};
    int fontSize = static_cast<int>(winSize.y * 0.04f);

    for (size_t i = 0; i < mainButtons.size(); ++i) {
        float y = startY + i * (buttonHeight + spacing);
        engine.drawRectangle(centerX, y, buttonWidth, buttonHeight, sf::Color::Blue);
        engine.drawText(mainButtons[i], centerX + buttonWidth * 0.3f, y + buttonHeight * 0.25f,
                        fontSize, sf::Color::White);
    }
}

void Menu::renderLoginInput() {
    engine.drawBackground();
    auto winSize = window->getSize();
    float inputWidth = winSize.x * 0.4f;
    float inputHeight = winSize.y * 0.06f;
    float centerX = winSize.x / 2.f;
    float titleY = winSize.y * 0.12f;
    int titleSize = static_cast<int>(winSize.y * 0.05f);
    float inputYStart = winSize.y * 0.35f;
    float spacing = winSize.y * 0.08f;
    int fontSize = static_cast<int>(winSize.y * 0.04f);

    // Titre centré
    {
        std::string title = "Connexion";
        float titleWidthApprox = title.length() * titleSize * 0.5f;
        float titleX = centerX - titleWidthApprox / 2.f;
        engine.drawText(title, titleX, titleY, titleSize, sf::Color::White);
    }

    // Champ nom d'utilisateur
    loginUsernameBox.setGeometry(centerX - inputWidth / 2.f, inputYStart, inputWidth, inputHeight);
    loginUsernameBox.render(engine);
    engine.drawIcon(ICON_CONNEXION, centerX - inputWidth / 2.f - inputHeight * 1.2f, inputYStart,
                    inputHeight, inputHeight);

    // Champ mot de passe
    loginPasswordBox.setGeometry(centerX - inputWidth / 2.f, inputYStart + spacing, inputWidth,
                                 inputHeight);
    loginPasswordBox.render(engine);
    engine.drawIcon(ICON_PASSWORD, centerX - inputWidth / 2.f - inputHeight * 0.7f,
                    inputYStart + spacing, inputHeight * 0.7f, inputHeight * 0.7f);
    engine.drawIcon(showLoginPassword ? ICON_EYE_OPEN : ICON_EYE_CLOSED,
                    centerX + inputWidth / 2.f + inputHeight * 0.2f, inputYStart + spacing,
                    inputHeight * 0.7f, inputHeight * 0.7f);

    // Bouton Login
    float loginBtnWidth = inputWidth * 0.4f;
    float loginBtnHeight = inputHeight;
    float loginBtnX = centerX - loginBtnWidth / 2.f;
    float loginBtnY = inputYStart + 2.f * spacing;
    engine.drawRectangle(loginBtnX, loginBtnY, loginBtnWidth, loginBtnHeight,
                         sf::Color(50, 200, 50));
    {
        std::string btnLabel = "Login";
        float textWidthApprox = btnLabel.length() * fontSize * 0.5f;
        float textX = loginBtnX + loginBtnWidth / 2.f - textWidthApprox / 2.f;
        float textY = loginBtnY + loginBtnHeight / 2.f - fontSize / 2.f;
        engine.drawText(btnLabel, textX, textY, fontSize, sf::Color::White);
    }

    // Icône retour
    float backBtnSize = winSize.y * 0.05f;
    engine.drawImage("../../../../assets/menu/back.png", winSize.x * 0.02f, winSize.y * 0.02f,
                     backBtnSize, backBtnSize);
}

void Menu::renderRegisterInput() {
    engine.drawBackground();
    auto winSize = window->getSize();
    float inputWidth = winSize.x * 0.4f;
    float inputHeight = winSize.y * 0.06f;
    float centerX = winSize.x / 2.f;
    float titleY = winSize.y * 0.12f;
    int titleSize = static_cast<int>(winSize.y * 0.05f);
    float inputYStart = winSize.y * 0.35f;
    float spacing = winSize.y * 0.08f;
    int fontSize = static_cast<int>(winSize.y * 0.04f);

    // Titre centré
    {
        std::string title = "Creer un compte";
        float titleWidthApprox = title.length() * titleSize * 0.5f;
        float titleX = centerX - titleWidthApprox / 2.f;
        engine.drawText(title, titleX, titleY, titleSize, sf::Color::Black);
    }

    // Champ nom d'utilisateur
    registerUsernameBox.setGeometry(centerX - inputWidth / 2.f, inputYStart, inputWidth,
                                    inputHeight);
    registerUsernameBox.render(engine);
    engine.drawIcon(ICON_CONNEXION, centerX - inputWidth / 2.f - inputHeight * 1.2f, inputYStart,
                    inputHeight, inputHeight);

    // Champ mot de passe
    registerPasswordBox.setGeometry(centerX - inputWidth / 2.f, inputYStart + spacing, inputWidth,
                                    inputHeight);
    registerPasswordBox.render(engine);
    engine.drawIcon(ICON_PASSWORD, centerX - inputWidth / 2.f - inputHeight * 0.7f,
                    inputYStart + spacing, inputHeight * 0.7f, inputHeight * 0.7f);
    engine.drawIcon(showRegisterPassword ? ICON_EYE_OPEN : ICON_EYE_CLOSED,
                    centerX + inputWidth / 2.f + inputHeight * 0.2f, inputYStart + spacing,
                    inputHeight * 0.7f, inputHeight * 0.7f);

    // Bouton S'inscrire
    float registerBtnWidth = inputWidth * 0.4f;
    float registerBtnHeight = inputHeight;
    float registerBtnX = centerX - registerBtnWidth / 2.f;
    float registerBtnY = inputYStart + 2.f * spacing;
    engine.drawRectangle(registerBtnX, registerBtnY, registerBtnWidth, registerBtnHeight,
                         sf::Color(50, 200, 50) // Moins vert fluo
    );
    {
        std::string btnLabel = "S'inscrire";
        float textWidthApprox = btnLabel.length() * fontSize * 0.5f;
        float textX = registerBtnX + registerBtnWidth / 2.f - textWidthApprox / 2.f;
        float textY = registerBtnY + registerBtnHeight / 2.f - fontSize / 2.f;
        engine.drawText(btnLabel, textX, textY, fontSize, sf::Color::White);
    }

    // Icône retour
    float backBtnSize = winSize.y * 0.05f;
    engine.drawImage("../../../../assets/menu/back.png", winSize.x * 0.02f, winSize.y * 0.02f,
                     backBtnSize, backBtnSize);
}

void Menu::renderQuitConfirmation() {
    engine.drawBackground();
    auto winSize = window->getSize();
    // Message centré
    int fontSize = static_cast<int>(winSize.y * 0.05f);
    std::string msg = response;
    float textWidthApprox = msg.length() * fontSize * 0.5f;
    float textX = winSize.x / 2.f - textWidthApprox / 2.f;
    float textY = winSize.y * 0.3f;
    engine.drawText(msg, textX, textY, fontSize, sf::Color::White);

    // Boutons Oui/Non dynamiques
    float btnWidth = winSize.x * 0.2f;
    float btnHeight = winSize.y * 0.08f;
    float btnY = winSize.y * 0.5f;
    float spacing = winSize.x * 0.05f;
    float centerX = winSize.x / 2.f;
    float ouiX = centerX - btnWidth - spacing / 2.f;
    float nonX = centerX + spacing / 2.f;

    // Bouton Oui
    engine.drawRectangle(ouiX, btnY, btnWidth, btnHeight, sf::Color::Red);
    {
        std::string label = "Oui";
        int lblSize = static_cast<int>(winSize.y * 0.04f);
        float lblWidthApprox = label.length() * lblSize * 0.5f;
        engine.drawText(label, ouiX + btnWidth / 2.f - lblWidthApprox / 2.f,
                        btnY + btnHeight / 2.f - lblSize / 2.f, lblSize, sf::Color::White);
    }
    // Bouton Non
    engine.drawRectangle(nonX, btnY, btnWidth, btnHeight,
                         sf::Color(50, 200, 50)); // Moins vert fluo;
    {
        std::string label = "Non";
        int lblSize = static_cast<int>(winSize.y * 0.04f);
        float lblWidthApprox = label.length() * lblSize * 0.5f;
        engine.drawText(label, nonX + btnWidth / 2.f - lblWidthApprox / 2.f,
                        btnY + btnHeight / 2.f - lblSize / 2.f, lblSize, sf::Color::White);
    }
}

// mode de jeux
void Menu::renderGameMenu() {
    // Affichage du menu de jeu
    engine.drawBackground();

    std::vector<std::string> gameButtons = {"Modes de jeux", "Gestion d'amis", "Liste des scores"};
    renderButton("    Menu de jeu", gameButtons);
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);
}

void Menu::renderGameMode() {

    engine.drawBackground();

    std::vector<std::string> gameModeButtons = {"Mode Solo (Endless)", "Mode Multijoueurs"};
    renderButton("  Modes de jeu", gameModeButtons);
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);
}

void Menu::renderSoloMode() {
    engine.drawBackground();

    // Afficher le pseudo du joueur et son meilleur score dans le coin gauche
    std::string pseudo = user;
    int bestScore = tetris::GlobalConnection::getInstance().request_score();

    std::vector<std::string> soloModeButtons = {"Jouer"};
    renderButton("     Mode Solo", soloModeButtons);
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);

    // Afficher le meilleur score sous le titre "Mode Solo"
    std::string bestScoreText = "MEILLEUR SCORE : " + std::to_string(bestScore);
    int fontSize = 24;
    float bestScoreX = window->getSize().x / 2.f - (bestScoreText.length() * 0.5f * fontSize);
    float bestScoreY = 150; // Position ajustée pour être sous le bouton "Jouer"
    engine.drawText(bestScoreText, bestScoreX + 110, bestScoreY, fontSize, sf::Color::Yellow);
}

void Menu::renderEndlessMode() {
    // Clear the background
    engine.drawBackground();

    // Draw back button
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);

    // Update and render endless game
    if (endlessGame && !endlessGame->wantsToReturnToMenu()) {
        // Update the game state
        endlessGame->update(0.016f); // Fixed update time (~ 60fps)
        endlessGame->render();
    } else {
        endlessGame.reset();
        currentState = MenuState::SoloMode;
    }
}

void Menu::renderMultiPlayerMode() {
    engine.drawBackground();
    std::vector<std::string> multiplayerModeButtons = {"Classic", "Duel", "Royal Competition"};
    renderButton("Mode Multijoueur", multiplayerModeButtons);
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);
}

void Menu::room_waiting(tetris::GameMode mode) {
    // Stocke le mode de jeu
    this->mode = mode;

    // Démarre le thread réseau de Room s'il n'est pas déjà actif et que nous avons un nom de room
    // valide ou si nous essayons d'en créer/rejoindre une. Room::createRoom/joinRoom s'en occupe.

    room.setGameMode(mode); // S'assurer que Room connaît le mode actuel

    // Clear messages when changing the room or entering this state
    // messages.clear(); // Peut-être pas ici, mais plutôt quand on quitte une room explicitement.

    // Affichage de l'interface
    engine.drawBackground();
    std::vector<std::string> optionsRoom = {"Creer une room", "Rafraichir", "Rejoindre une room",
                                            "Discussion", "Continuer"};
    renderButton("   Salle d'attente", optionsRoom);

    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);

    std::string roomName = room.getRoomName();
    if (roomName.empty() || roomName.rfind("Tentative de connexion à ", 0) == 0) {
        engine.drawText(roomName.empty() ? "Aucune room actuelle" : roomName, 50, 100, 18,
                        sf::Color::White);
    } else {
        engine.drawText("Room actuelle : " + roomName, 50, 100, 18, sf::Color::White);
    }
    engine.drawText("Nombre de joueurs : " + std::to_string(room.getPlayerCount()), 50, 120, 18,
                    sf::Color::White);

    // Affichage du mode de jeu choisi
    // Utiliser room.getGameMode() car `this->mode` pourrait être différent si on a juste rejoint
    tetris::GameMode currentRoomGameMode = room.getGameMode();
    if (currentRoomGameMode == tetris::GameMode::Classique) {
        engine.drawText("Mode : Classique", 50, 140, 18, sf::Color::White);
    } else if (currentRoomGameMode == tetris::GameMode::Duel) {
        engine.drawText("Mode : Duel", 50, 140, 18, sf::Color::White);
    } else if (currentRoomGameMode == tetris::GameMode::BR) {
        engine.drawText("Mode : Royal Competition", 50, 140, 18, sf::Color::White);
    } else if (roomName.empty()) {
        // Pas de mode à afficher si pas dans une room
    } else {
        engine.drawText("Mode : Inconnu", 50, 140, 18, sf::Color::Yellow);
    }
}

void Menu::renderDiscussion() {
    engine.drawBackground();
    auto winSize = window->getSize();
    float inputWidth = winSize.x * 0.4f;
    float inputHeight = winSize.y * 0.06f;
    float margin = winSize.x * 0.02f;                 // Margin from the edges
    float bottomY = winSize.y - inputHeight - margin; // Bottom position with margin
    int fontSize = static_cast<int>(winSize.y * 0.04f);

    // Titre centré
    {
        std::string title = "Discussion";
        float titleWidthApprox = title.length() * fontSize * 0.5f;
        float titleX = winSize.x / 2.f - titleWidthApprox / 2.f;
        float titleY = winSize.y * 0.12f;
        engine.drawText(title, titleX, titleY, fontSize, sf::Color::White);
    }

    // Champ de saisie (roomChatBox) en bas à gauche
    roomChatBox.setGeometry(margin, bottomY - inputHeight - margin, inputWidth, inputHeight);
    roomChatBox.render(engine);

    // Bouton Envoyer à droite du roomChatBox
    float sendBtnWidth = inputWidth * 0.4f;
    float sendBtnHeight = inputHeight;
    float sendBtnX = margin + inputWidth + margin; // Positioned next to roomChatBox
    float sendBtnY = bottomY - inputHeight - margin;
    engine.drawRectangle(sendBtnX, sendBtnY, sendBtnWidth, sendBtnHeight, sf::Color(50, 200, 50));

    {
        std::string btnLabel = "Envoyer";
        float textWidthApprox = btnLabel.length() * fontSize * 0.5f;
        float textX = sendBtnX + sendBtnWidth / 2.f - textWidthApprox / 2.f;
        float textY = sendBtnY + sendBtnHeight / 2.f - fontSize / 2.f;
        engine.drawText(btnLabel, textX, textY, fontSize, sf::Color::White);
    }

    // Icône retour
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);

    // Récupérer les messages depuis l'objet room
    const auto &messagesFromRoom = room.getMessages();

    // Affichage de la liste des messages
    float messageStartY = 120; // Position Y de départ pour les messages
    float messageSpacing = 30; // Espacement entre les messages

    engine.drawText("Messages recus: " + std::to_string(messagesFromRoom.size()), 50, 100, 16,
                    sf::Color::Yellow);

    for (size_t i = 0; i < messagesFromRoom.size(); ++i) {
        engine.drawText(messagesFromRoom[i], 50, messageStartY + i * messageSpacing, 24,
                        sf::Color::White);
    }
}

void Menu::renderCreateRoom() {
    engine.drawBackground();
    engine.drawText("Creer une room", 320, 120, 32, sf::Color::White);
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);

    createRoomBox.render(engine);

    engine.drawRectangle(340, 410, 150, 50, sf::Color(50, 200, 50));
    engine.drawText("Creer", 355, 425, 20, sf::Color::White);
}

void Menu::renderCreateRoomResponse() {
    engine.drawBackground();

    std::string roomName = room.getRoomName();
    if (!roomName.empty()) {
        engine.drawText("Room ouverte : " + roomName, 250, 120, 32,
                        sf::Color(50, 200, 50)); // Moins vert fluo;
        engine.drawText("Nombre de joueurs: " + std::to_string(room.getPlayerCount()), 275, 170, 24,
                        sf::Color::White);
    } else {
        engine.drawText(response, 250, 120, 32, sf::Color::White);
    }

    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);
    engine.drawRectangle(320, 205, 150, 50, sf::Color::Blue);
    engine.drawText("OK", 380, 215, 24, sf::Color::White);
}

void Menu::renderJoinRoom() {
    engine.drawBackground();
    engine.drawText("Rejoindre une room", 270, 120, 32, sf::Color::White);
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);

    joinRoomBox.render(engine);

    engine.drawRectangle(340, 410, 150, 50, sf::Color(50, 200, 50)); // Moins vert fluo;
    engine.drawText("Rejoindre", 355, 425, 20, sf::Color::White);
}

void Menu::renderJoinRoomResponse() {
    engine.drawBackground();

    std::string current_room = room.getRoomName();
    if (!current_room.empty()) {
        engine.drawText("Room rejointe: " + current_room, 250, 120, 32,
                        sf::Color(50, 200, 50)); // Moins vert fluo;
        engine.drawText("Nombre de joueurs: " + std::to_string(room.getPlayerCount()), 275, 170, 24,
                        sf::Color::White);
    } else {
        engine.drawText(response, 250, 120, 32, sf::Color::White);
    }

    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);
    engine.drawRectangle(320, 210, 150, 50, sf::Color::Blue);
    engine.drawText("OK", 370, 225, 24, sf::Color::White);
}

void Menu::hostStartGame() {
    engine.drawBackground();

    // if (user == hote){

    bool isUserHost = user == room.getHost();

    int players = room.getPlayerCount();

    if (isUserHost) {
        std::vector<std::string> hostStart = {"Lancer la partie"};

        // Affichage du bouton de lancement mode de jeu
        if (mode == tetris::GameMode::Classique) {
            if (2 <= players && players <= 9) {
                renderButton("Mode : Classique", hostStart);
            } else if (players < 2) {
                engine.drawText("Nombres de joueurs insuffisant", 250, 120, 32, sf::Color::White);
            } else if (players > 9) {
                engine.drawText("Trop de joueurs dans la room", 250, 120, 32, sf::Color::White);
            }

        } else if (mode == tetris::GameMode::Duel) {
            if (players == 2) {
                renderButton("Mode : Duel", hostStart);
            } else if (players < 2) {
                engine.drawText("Nombres de joueurs insuffisant", 250, 120, 32, sf::Color::White);
            } else if (players > 2) {
                engine.drawText("Trop de joueurs dans la room", 250, 120, 32, sf::Color::White);
            }
        } else if (mode == tetris::GameMode::BR) {
            if (2 <= players && players <= 9) {
                renderButton("Mode : Royal Competition", hostStart);
            } else if (players < 2) {
                engine.drawText("Nombres de joueurs insuffisant", 250, 120, 32, sf::Color::White);
            } else if (players > 9) {
                engine.drawText("Trop de joueurs dans la room", 250, 120, 32, sf::Color::White);
            }
        }
    } else {
        engine.drawText("En attente de l'host", 250, 120, 32, sf::Color::White);
    }

    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);
}

void Menu::renderFriendsMenu() {
    engine.drawBackground();
    std::vector<std::string> friendsButtons = {"Ajouter un ami", "Supprimer un ami",
                                               "Afficher la liste d'amis", "Demandes d'amis",
                                               "Messagerie"};
    renderButton("Gestion des amis", friendsButtons);
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);
}

void Menu::renderAddFriends() {
    engine.drawBackground();
    auto winSize = window->getSize();
    float inputWidth = winSize.x * 0.4f;
    float inputHeight = winSize.y * 0.06f;
    float centerX = winSize.x / 2.f;
    float titleY = winSize.y * 0.12f;
    int titleSize = static_cast<int>(winSize.y * 0.05f);
    float inputYStart = winSize.y * 0.35f;
    float spacing = winSize.y * 0.08f;

    // Titre centré
    {
        std::string title = "Ajouter un ami";
        float widthApprox = title.length() * titleSize * 0.5f;
        float titleX = centerX - widthApprox / 2.f;
        engine.drawText(title, titleX, titleY, titleSize, sf::Color::White);
    }

    // Back icon
    float backSize = winSize.y * 0.05f;
    engine.drawImage(ASSETS_SRC "/menu/back.png", winSize.x * 0.02f, winSize.y * 0.02f, backSize,
                     backSize);

    // Champ de saisie
    addFriendBox.setGeometry(centerX - inputWidth / 2.f, inputYStart, inputWidth, inputHeight);
    addFriendBox.render(engine);
    engine.drawIcon(ICON_CONNEXION, centerX - inputWidth / 2.f - inputHeight * 1.2f, inputYStart,
                    inputHeight, inputHeight);

    // Bouton Ajouter
    float btnWidth = inputWidth * 0.4f;
    float btnHeight = inputHeight;
    float btnX = centerX - btnWidth / 2.f;
    float btnY = inputYStart + 2.f * spacing;
    engine.drawRectangle(btnX, btnY, btnWidth, btnHeight,
                         sf::Color(50, 200, 50)); // Moins vert fluo;
    {
        std::string btnLabel = "Ajouter";
        float txtW = btnLabel.length() * titleSize * 0.5f;
        engine.drawText(btnLabel, btnX + btnWidth / 2.f - txtW / 2.f,
                        btnY + btnHeight / 2.f - titleSize / 2.f, titleSize, sf::Color::White);
    }
}

void Menu::renderRemoveFriends() {
    engine.drawBackground();
    auto winSize = window->getSize();
    float inputWidth = winSize.x * 0.4f;
    float inputHeight = winSize.y * 0.06f;
    float centerX = winSize.x / 2.f;
    float titleY = winSize.y * 0.12f;
    int titleSize = static_cast<int>(winSize.y * 0.05f);
    float inputYStart = winSize.y * 0.35f;
    float spacing = winSize.y * 0.08f;

    // Titre centré
    {
        std::string title = "Supprimer un ami";
        float widthApprox = title.length() * titleSize * 0.5f;
        float titleX = centerX - widthApprox / 2.f;
        engine.drawText(title, titleX, titleY, titleSize, sf::Color::White);
    }

    // Icône retour
    float backSize = winSize.y * 0.05f;
    engine.drawImage(ASSETS_SRC "/menu/back.png", winSize.x * 0.02f, winSize.y * 0.02f, backSize,
                     backSize);

    // Champ de saisie
    removeFriendBox.setGeometry(centerX - inputWidth / 2.f, inputYStart, inputWidth, inputHeight);
    removeFriendBox.render(engine);
    engine.drawIcon(ICON_CONNEXION, centerX - inputWidth / 2.f - inputHeight * 1.2f, inputYStart,
                    inputHeight, inputHeight);

    // Bouton Supprimer
    float btnWidth = inputWidth * 0.4f;
    float btnHeight = inputHeight;
    float btnX = centerX - btnWidth / 2.f;
    float btnY = inputYStart + 2.f * spacing;
    engine.drawRectangle(btnX, btnY, btnWidth, btnHeight, sf::Color::Red);
    {
        std::string btnLabel = "Supprimer";
        float txtW = btnLabel.length() * titleSize * 0.5f;
        engine.drawText(btnLabel, btnX + btnWidth / 2.f - txtW / 2.f,
                        btnY + btnHeight / 2.f - titleSize / 2.f, titleSize, sf::Color::White);
    }
}

void Menu::renderFriendList() {
    engine.drawBackground();

    // Récupérer la liste d'amis
    friendsList = tetris::GlobalConnection::getInstance().friends_list();
    int totalFriends = friendsList.size();

    // Taille de la fenêtre
    auto winSize = window->getSize();
    const float centerX = winSize.x / 2.f;

    // Titre centré
    int titleFontSize = static_cast<int>(winSize.y * 0.05f);
    std::string title = "Liste d'ami(e)s : " + std::to_string(totalFriends);
    float titleWidthApprox = title.length() * titleFontSize * 0.5f;
    float titleX = centerX - titleWidthApprox / 2.f;
    engine.drawText(title, titleX, 40, titleFontSize, sf::Color::White);

    // Bouton retour
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);

    // En-tête
    int fontSize = 24;

    // Affichage des amis
    int yStart = 120;
    int lineHeight = 50;

    for (size_t i = 0; i < friendsList.size(); ++i) {
        std::string friendName = "- " + friendsList[i];

        // Afficher le nom de l'ami
        engine.drawText(friendName, centerX - 100, yStart + i * lineHeight, fontSize,
                        sf::Color::White);
    }
}

void Menu::renderManageFriendsRequests() {
    engine.drawBackground();
    friendRequests = tetris::GlobalConnection::getInstance().handle_my_requests();
    int totalRequests = friendRequests.size();

    // Centrer le titre "Demandes d'amis"
    auto winSize = window->getSize();
    int titleFontSize = static_cast<int>(winSize.y * 0.05f);
    std::string title = "Demandes d'amis : " + std::to_string(totalRequests);
    float titleWidthApprox = title.length() * titleFontSize * 0.5f;
    float titleX = winSize.x / 2.f - titleWidthApprox / 2.f;
    engine.drawText(title, titleX, 120, titleFontSize, sf::Color::White);
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);

    for (size_t i = 0; i < friendRequests.size(); ++i) {
        std::string request = friendRequests[i];

        // Position de la demande
        int textX = winSize.x / 2.f - 200; // Centrer horizontalement
        int textY = 200 + (i * 70);        // Espacement vertical entre les demandes

        // Afficher la demande
        engine.drawText(request, textX, textY, 32, sf::Color::White);

        // Boutons Oui/Non
        float btnWidth = 100;
        float btnHeight = 40;
        float btnSpacing = 20;
        float ouiX = textX + 250;
        float nonX = ouiX + btnWidth + btnSpacing;

        engine.drawRectangle(ouiX, textY, btnWidth, btnHeight,
                             sf::Color(50, 200, 50)); // Moins vert fluo;
        engine.drawText("Oui", ouiX + 30, textY + 10, 24, sf::Color::White);

        engine.drawRectangle(nonX, textY, btnWidth, btnHeight, sf::Color::Red);
        engine.drawText("Non", nonX + 30, textY + 10, 24, sf::Color::White);
    }
}

void Menu::renderFriendsRequestsResponse() {
    engine.drawBackground();
    engine.drawText(response, 320, 120, 32, sf::Color::White);
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);

    engine.drawRectangle(320, 200, 150, 50, sf::Color::Blue);
    engine.drawText("OK", 370, 215, 24, sf::Color::White);
}

void Menu::renderTopScores() {
    engine.drawBackground();
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);

    topScores = tetris::GlobalConnection::getInstance().top_scores_list();
    const float centerX = window->getSize().x / 2.f;

    if (topScores.empty()) {
        engine.drawText("Aucun score disponible", centerX - 150, 200, 28, sf::Color::White);
        return;
    }

    // Affichage du titre
    engine.drawText("TOP SCORES", centerX - 100, 40, 36, sf::Color::White);

    // En-têtes
    int fontSize = 24;
    float pseudoColX = centerX - 200;
    float scoreColX = centerX + 200;

    engine.drawText("Pseudo", pseudoColX - 40, 120, fontSize, sf::Color::White);
    engine.drawText("Score", scoreColX - 30, 120, fontSize, sf::Color::White);

    // Affichage des scores
    int yStart = 160;
    int lineHeight = 50;

    for (size_t i = 0; i < topScores.size(); ++i) {
        size_t pos = topScores[i].find(" : ");
        if (pos == std::string::npos)
            continue;

        std::string pseudo = topScores[i].substr(0, pos);
        std::string score = topScores[i].substr(pos + 3);

        sf::Color color = (i < 3) ? sf::Color(255, 215, 0) : sf::Color::White;
        float y = yStart + i * lineHeight;

        engine.drawText(pseudo, pseudoColX - pseudo.length() * fontSize * 0.25f, y, fontSize,
                        color);
        engine.drawText(score, scoreColX - score.length() * fontSize * 0.25f, y, fontSize, color);
    }
}

void Menu::renderClassicGameActive() {
    // Clear the background
    engine.drawBackground();

    // Draw back button
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);

    // Update and render classic game
    if (classicGame) {
        classicGame->update(0.016f); // Fixed update time (~ 60fps)
        classicGame->render();
        if (classicGame->wantsToReturnToMenu()) {
            classicGame.reset();
            room.leaveRoom();
            currentState = MenuState::MultiPlayerMode;
        }
    }
}

void Menu::renderDuelModeActive() {
    // Clear the background
    engine.drawBackground();

    // Draw back button
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);

    // Update and render duel game
    if (duelGame) {
        duelGame->update(0.016f); // Fixed update time (~ 60fps)
        duelGame->render();
        if (duelGame->wantsToReturnToMenu()) {
            duelGame.reset();
            room.leaveRoom();
            currentState = MenuState::MultiPlayerMode; //**** */
        }
    }
}

void Menu::renderRoyalCompetitionActive() {
    // Clear the background
    engine.drawBackground();

    // Draw back button
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);

    // Update and render royal competition game
    if (royalCompetitionGame) {
        royalCompetitionGame->update(0.016f); // Fixed update time (~ 60fps)
        royalCompetitionGame->render();
        if (royalCompetitionGame->wantsToReturnToMenu()) {
            royalCompetitionGame.reset();
            room.leaveRoom();
            currentState = MenuState::MultiPlayerMode; //**** */
        }
    }
}

void Menu::handleEvents(const sf::Event &event) {
    if (event.is<sf::Event::Resized>()) {
        const auto *resized = event.getIf<sf::Event::Resized>();
        sf::Vector2f newSize(static_cast<float>(resized->size.x),
                             static_cast<float>(resized->size.y));
        sf::View view;
        view.setSize(newSize);
        view.setCenter({newSize.x / 2.f, newSize.y / 2.f});
        window->setView(view);
        engine.loadBackground(ICON_BACKGROUND, resized->size.x, resized->size.y);
        return;
    }
    int buttonWidth = 200;
    int buttonHeight = 50;
    int spacing = 20;
    int startY = 200;
    int centerX = window->getSize().x / 2 - buttonWidth / 2;

    if (currentState == MenuState::MainMenu) {
        auto winSizeHE = window->getSize();
        float buttonWidth = winSizeHE.x * 0.3f;
        float buttonHeight = winSizeHE.y * 0.08f;
        float spacing = winSizeHE.y * 0.05f;
        float startY = winSizeHE.y * 0.4f;
        float centerX = winSizeHE.x / 2.f - buttonWidth / 2.f;

        // Bouton Connexion
        if (engine.getMouseHandler().clickedInRectangle(centerX, startY, buttonWidth,
                                                        buttonHeight)) {
            currentState = MenuState::LoginInput;
            return;
        }
        // Bouton S'inscrire
        else if (engine.getMouseHandler().clickedInRectangle(
                     centerX, startY + (buttonHeight + spacing), buttonWidth, buttonHeight)) {
            currentState = MenuState::RegisterInput;
            return;
        }
        // Bouton Quitter
        else if (engine.getMouseHandler().clickedInRectangle(
                     centerX, startY + 2.f * (buttonHeight + spacing), buttonWidth, buttonHeight)) {
            response = "Voulez-vous vraiment quitter ?";
            float textWidthApprox =
                response.length() * 0.5f * static_cast<int>(window->getSize().y * 0.05f);
            float textX = window->getSize().x / 2.f - textWidthApprox / 2.f;
            float textY = window->getSize().y * 0.3f;
            engine.drawText(response, textX, textY, static_cast<int>(window->getSize().y * 0.05f),
                            sf::Color::White);
            currentState = MenuState::QuitConfirmation;
        }
    }

    if (currentState == MenuState::LoginInput) {
        // Dynamic layout values
        auto winSize = window->getSize();
        float inputWidth = winSize.x * 0.4f;
        float inputHeight = winSize.y * 0.06f;
        float centerX = winSize.x / 2.f;
        float inputYStart = winSize.y * 0.35f;
        float spacing = winSize.y * 0.08f;
        float backBtnSize = winSize.y * 0.05f;
        float backX = winSize.x * 0.02f;
        float backY = winSize.y * 0.02f;

        // Back icon click
        if (engine.getMouseHandler().clickedInRectangle(backX, backY, backBtnSize, backBtnSize)) {
            resetInputFields();
            currentState = MenuState::MainMenu;
            return;
        }

        // Input handling
        loginUsernameBox.handleEvent(event, engine.getMouseHandler());
        loginPasswordBox.handleEvent(event, engine.getMouseHandler());
        handleEyeClick(showLoginPassword, loginPasswordBox,
                       centerX + inputWidth / 2.f + inputHeight * 0.2f, inputYStart + spacing);

        // Login button click
        float loginBtnWidth = inputWidth * 0.4f;
        float loginBtnHeight = inputHeight;
        float loginBtnX = centerX - loginBtnWidth / 2.f;
        float loginBtnY = inputYStart + 2.f * spacing;
        if (engine.getMouseHandler().clickedInRectangle(loginBtnX, loginBtnY, loginBtnWidth,
                                                        loginBtnHeight)) {
            user = loginUsernameBox.getText();
            pass = loginPasswordBox.getText();
            if (user.empty()) {
                loginUsernameBox.setErrorMessage("Nom d'utilisateur non valide");
                return renderLoginInput();
            }
            if (pass.empty()) {
                loginPasswordBox.setErrorMessage("Mot de passe non valide");
                return renderLoginInput();
            }
            if (tetris::GlobalConnection::getInstance().connect_log(user, pass)) {
                currentState = MenuState::GameMenu;
            } else {
                loginPasswordBox.setErrorMessage(
                    "Connexion impossible : vérifier serveur et identifiants");
            }
            return;
        }
    }

    if (currentState == MenuState::RegisterInput) {
        // Dynamic layout values
        auto winSize = window->getSize();
        float inputWidth = winSize.x * 0.4f;
        float inputHeight = winSize.y * 0.06f;
        float centerX = winSize.x / 2.f;
        float inputYStart = winSize.y * 0.35f;
        float spacing = winSize.y * 0.08f;
        float backBtnSize = winSize.y * 0.05f;
        float backX = winSize.x * 0.02f;
        float backY = winSize.y * 0.02f;

        // Back icon click
        if (engine.getMouseHandler().clickedInRectangle(backX, backY, backBtnSize, backBtnSize)) {
            resetInputFields();
            currentState = MenuState::MainMenu;
            return;
        }

        // Input handling
        registerUsernameBox.handleEvent(event, engine.getMouseHandler());
        registerPasswordBox.handleEvent(event, engine.getMouseHandler());
        handleEyeClick(showRegisterPassword, registerPasswordBox,
                       centerX + inputWidth / 2.f + inputHeight * 0.2f, inputYStart + spacing);

        // Register button click
        float registerBtnWidth = inputWidth * 0.4f;
        float registerBtnHeight = inputHeight;
        float registerBtnX = centerX - registerBtnWidth / 2.f;
        float registerBtnY = inputYStart + 2.f * spacing;
        if (engine.getMouseHandler().clickedInRectangle(registerBtnX, registerBtnY,
                                                        registerBtnWidth, registerBtnHeight)) {
            user = registerUsernameBox.getText();
            pass = registerPasswordBox.getText();
            if (user.empty()) {
                registerUsernameBox.setErrorMessage("Nom d'utilisateur requis");
                return renderRegisterInput();
            }
            if (pass.empty()) {
                registerPasswordBox.setErrorMessage("Mot de passe requis");
                return renderRegisterInput();
            }
            if (tetris::GlobalConnection::getInstance().connect_create(user, pass)) {
                currentState = MenuState::GameMenu;
            } else {
                registerUsernameBox.setErrorMessage(
                    "Inscription impossible : vérifier serveur et nom");
                return renderRegisterInput();
            }
            return;
        }
    }

    if (currentState == MenuState::QuitConfirmation) {
        auto winSize = window->getSize();
        float btnWidth = winSize.x * 0.2f;
        float btnHeight = winSize.y * 0.08f;
        float btnY = winSize.y * 0.5f;
        float spacing = winSize.x * 0.05f;
        float centerX = winSize.x / 2.f;
        float ouiX = centerX - btnWidth - spacing / 2.f;
        float nonX = centerX + spacing / 2.f;

        if (engine.getMouseHandler().clickedInRectangle(ouiX, btnY, btnWidth, btnHeight)) {
            currentState = MenuState::QUIT;
            return;
        }
        if (engine.getMouseHandler().clickedInRectangle(nonX, btnY, btnWidth, btnHeight)) {
            currentState = MenuState::MainMenu;
            return;
        }
    }

    if (currentState == MenuState::GameMenu) {

        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20)) {
            currentState = MenuState::MainMenu;
            return;
        }

        else if (engine.getMouseHandler().clickedInRectangle(centerX, startY, buttonWidth,
                                                             buttonHeight)) {
            currentState = MenuState::GameModeMenu;
            return;
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + buttonHeight + spacing, buttonWidth, buttonHeight)) {
            currentState = MenuState::FriendsMenu;
            return;
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + 2 * (buttonHeight + spacing), buttonWidth, buttonHeight)) {
            currentState = MenuState::TopScores;
            return;
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + 3 * (buttonHeight + spacing), buttonWidth, buttonHeight)) {
            currentState = MenuState::MainMenu; // Retour au menu principal
        }
    }

    if (currentState == MenuState::GameModeMenu) {

        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20)) {
            currentState = MenuState::GameMenu;
            return;
        }

        if (engine.getMouseHandler().clickedInRectangle(centerX, startY, buttonWidth,
                                                        buttonHeight)) {
            currentState = MenuState::SoloMode;
            return;
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + buttonHeight + spacing, buttonWidth, buttonHeight)) {
            currentState = MenuState::MultiPlayerMode;
            return;
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + 2 * (buttonHeight + spacing), buttonWidth, buttonHeight)) {
            currentState = MenuState::GameMenu;
        }
    }

    if (currentState == MenuState::SoloMode) {
        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20)) {
            currentState = MenuState::GameModeMenu;
            return;
        }

        if (engine.getMouseHandler().clickedInRectangle(centerX, startY, buttonWidth,
                                                        buttonHeight)) {
            // Instead of setting startGame flag, create and start EndlessGUI
            endlessGame = std::make_unique<tetris::gui::EndlessGUI>(engine, window);
            endlessGame->start();
            currentState = MenuState::EndlessMode;
            return;
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + buttonHeight + spacing, buttonWidth, buttonHeight)) {
            currentState = MenuState::GameModeMenu;
        }
    }

    if (currentState == MenuState::MultiPlayerMode) {

        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20)) {
            // stopNetworkThread(); // Ensure thread is stopped when going back

            room.leaveRoom();
            currentState = MenuState::GameModeMenu;
            return;
        }

        // Classic mode
        else if (engine.getMouseHandler().clickedInRectangle(centerX, startY, buttonWidth,
                                                             buttonHeight)) {
            mode = tetris::GameMode::Classique;
            currentState = MenuState::RoomWaiting;
            return;

            // Duel mode
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + buttonHeight + spacing, buttonWidth, buttonHeight)) {
            mode = tetris::GameMode::Duel;
            currentState = MenuState::RoomWaiting;
            return;

            // Royal Competition mode
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + 2 * (buttonHeight + spacing), buttonWidth, buttonHeight)) {
            mode = tetris::GameMode::BR;
            currentState = MenuState::RoomWaiting;
            return;
        }
    }

    if (currentState == MenuState::RoomWaiting) {
        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20)) {

            room.leaveRoom();
            currentState = MenuState::MultiPlayerMode;
            return;
        }

        else if (engine.getMouseHandler().clickedInRectangle(centerX, startY, buttonWidth,
                                                             buttonHeight)) {
            currentState = MenuState::CreateRoom;
            return;
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + buttonHeight + spacing, buttonWidth, buttonHeight)) {
            room.setOnRoomUpdateCallback([this]() {
                // Update using the room's current state
                current_room = room.getRoomName();
                player_in_room = room.getPlayerCount();
            });
            return;
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + 2 * (buttonHeight + spacing), buttonWidth, buttonHeight)) {
            currentState = MenuState::JoinRoom;
            return;
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + 3 * (buttonHeight + spacing), buttonWidth, buttonHeight)) {
            currentState = MenuState::Discussion;
            return;
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + 4 * (buttonHeight + spacing), buttonWidth, buttonHeight)) {
            currentState = MenuState::HostStartGame;
            return;
        }
    }

    if (currentState == MenuState::CreateRoom) {
        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20)) {
            resetInputFields();
            currentState = MenuState::RoomWaiting;
            return;
        }

        createRoomBox.handleEvent(event, engine.getMouseHandler());
        if (engine.getMouseHandler().clickedInRectangle(340, 410, 150, 50)) {
            std::string roomName = createRoomBox.getText();
            if (roomName.empty()) {
                createRoomBox.setErrorMessage("Nom de la room requis");
                return;
            }

            // Démarre le thread réseau s'il n'est pas déjà actif
            /*if (!threadRunning) {
                startNetworkThread();
            }

            // Création de la room
            tetris::GlobalConnection::getInstance().create_room(maxPlayer, roomName, mode);
            //std::cout << "Attempting to create room: " << roomName << " with mode: " <<
            static_cast<int>(mode) << std::endl;

            response = "Room faite";*/

            if (room.createRoom(roomName, mode)) {
                room.setHost(user); // L'utilisateur actuel devient l'hôte
                response = "Room créée avec succès";
            } else {
                response = "Échec de création de la room";
            }

            // hote = user;
            currentState = MenuState::CreateRoomResponse;
            return;
        }
    }

    if (currentState == MenuState::CreateRoomResponse) {
        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20) ||
            engine.getMouseHandler().clickedInRectangle(320, 200, 150, 50)) {
            currentState = MenuState::RoomWaiting;
            return;
        }
    }

    if (currentState == MenuState::JoinRoom) {
        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20)) {
            resetInputFields();
            currentState = MenuState::RoomWaiting;
            return;
        }

        joinRoomBox.handleEvent(event, engine.getMouseHandler());
        if (engine.getMouseHandler().clickedInRectangle(340, 410, 150, 50)) {
            std::string roomName = joinRoomBox.getText();
            if (roomName.empty()) {
                joinRoomBox.setErrorMessage("Nom de la room requis");
                return;
            }

            // Démarre le thread réseau s'il n'est pas déjà actif
            /*if (!threadRunning) {
                startNetworkThread();
            }

            // Rejoindre la room avec plusieurs tentatives si nécessaire
            //std::cout << "Attempting to join room: " << roomName << std::endl;
            bool joinSuccess = false;

            // Première tentative
            tetris::GlobalConnection::getInstance().join_room(roomName);

            // Attends un peu pour la réponse
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            // Vérifie si on a rejoint avec succès
            std::string currentRoom = tetris::GlobalConnection::getInstance().get_current_room();
            if (!currentRoom.empty()) {
                joinSuccess = true;
                response = "Room rejointe : " + currentRoom;
                std::cout << response << std::endl;*/

            // Rejoindre la room
            if (room.joinRoom(roomName)) {
                response = "Room rejointe avec succès";

            } else {
                // Deuxième tentative
                /*tetris::GlobalConnection::getInstance().join_room(roomName);
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                currentRoom = tetris::GlobalConnection::getInstance().get_current_room();

                if (!currentRoom.empty()) {
                    joinSuccess = true;
                    response = "Room rejointe : " + currentRoom;
                    std::cout << response << std::endl;
                } else {
                    response = "Tentative de rejoindre la room " + roomName;
                    std::cout << "Failed to join room: " << roomName << std::endl;
                }*/
                response = "Échec lors de la tentative de rejoindre la room";
            }

            currentState = MenuState::JoinRoomResponse;
            return;
        }
    }

    if (currentState == MenuState::JoinRoomResponse) {
        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20) ||
            engine.getMouseHandler().clickedInRectangle(320, 200, 150, 50)) {
            currentState = MenuState::RoomWaiting;
            return;
        }
    }

    if (currentState == MenuState::HostStartGame) {
        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20)) {
            // stopNetworkThread(); // Stop thread when leaving room state
            currentState = MenuState::RoomWaiting;
            return;
        }

        if (engine.getMouseHandler().clickedInRectangle(centerX, startY, buttonWidth,
                                                        buttonHeight)) {
            /*if (user == hote) { // Only host can start the game
                //std::cout << "Host clicked on 'Lancer la partie' button" << std::endl;
                tetris::GlobalConnection::getInstance().start_game(mode);

                // Au lieu de forcer le démarrage, laissons le thread_lecture
                // détecter le paquet GameStart et lancer le jeu naturellement
                //std::cout << "Waiting for GameStart packet from server..." << std::endl;
            }*/

            if (room.isHost(user)) { // Only host can start the game
                if (room.startGame(mode)) {
                    // Le jeu démarrera automatiquement via room.shouldStartGame()
                    std::cout << "Game start request sent" << std::endl;
                } else {
                    std::cout << "Failed to start game" << std::endl;
                }
            }
        }
    }

    if (currentState == MenuState::Discussion) {
        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20)) {
            resetInputFields();
            currentState = MenuState::RoomWaiting;
            return;
        }

        auto winSize = window->getSize();
        float inputWidth = winSize.x * 0.4f;
        float inputHeight = winSize.y * 0.06f;
        float margin = winSize.x * 0.02f;                 // Margin from the edges
        float bottomY = winSize.y - inputHeight - margin; // Bottom position with margin
        int fontSize = static_cast<int>(winSize.y * 0.04f);

        float sendBtnWidth = inputWidth * 0.4f;
        float sendBtnHeight = inputHeight;
        float sendBtnX = margin + inputWidth + margin; // Positioned next to roomChatBox
        float sendBtnY = bottomY - inputHeight - margin;

        roomChatBox.handleEvent(event, engine.getMouseHandler());
        if (engine.getMouseHandler().clickedInRectangle(sendBtnX, sendBtnY, sendBtnWidth,
                                                        sendBtnHeight)) {
            std::string messageRoom = roomChatBox.getText();
            if (messageRoom.empty()) {
                roomChatBox.setErrorMessage("Message requis");
                return;
            }

            // Utiliser le nom de la room actuelle à partir de l'objet room
            std::string roomName = room.getRoomName();
            if (!roomName.empty()) {
                // Envoyer le message avec le bon nom de room
                tetris::GlobalConnection::getInstance().send_message_room(roomName, messageRoom);
                std::cout << "Message envoyé à la room: " << roomName << std::endl;
            } else {
                std::cout << "Impossible d'envoyer le message: nom de room non disponible"
                          << std::endl;
            }
        }
    }

    if (currentState == MenuState::FriendsMenu) {

        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20)) {
            currentState = MenuState::GameMenu;
            return;
        } else if (engine.getMouseHandler().clickedInRectangle(centerX, startY, buttonWidth,
                                                               buttonHeight)) {
            currentState = MenuState::AddFriends;
            return;
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + buttonHeight + spacing, buttonWidth, buttonHeight)) {
            currentState = MenuState::RemoveFriends;
            return;
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + 2 * (buttonHeight + spacing), buttonWidth, buttonHeight)) {
            currentState = MenuState::FriendsList;
            return;
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + 3 * (buttonHeight + spacing), buttonWidth, buttonHeight)) {
            currentState = MenuState::ManageFriendsRequests;
            return;
        } else if (engine.getMouseHandler().clickedInRectangle(
                       centerX, startY + 4 * (buttonHeight + spacing), buttonWidth, buttonHeight)) {
            currentState = MenuState::Messaging;
            return;
        }
    }

    if (currentState == MenuState::Messaging) {
        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20)) {
            currentState = MenuState::FriendsMenu;
            return;
        }

        sendMessageToBox.handleEvent(event, engine.getMouseHandler());
        sendMessageContentBox.handleEvent(event, engine.getMouseHandler());

        if (engine.getMouseHandler().clickedInRectangle(100, 330, 100, 40)) {
            std::string recipient = sendMessageToBox.getText();
            std::string content = sendMessageContentBox.getText();

            if (!recipient.empty() && !content.empty()) {
                auto &connection = tetris::GlobalConnection::getInstance();
                connection.send_message(recipient, content);
                response = connection.isConnected()
                               ? "Message envoye a " + recipient
                               : "Connexion perdue. Reconnectez-vous pour envoyer un message.";
            } else {
                response = "Veuillez remplir les deux champs.";
            }

            currentState = MenuState::FriendsRequestsResponse;
            return;
        }
    }

    if (currentState == MenuState::AddFriends) {
        auto winSize = window->getSize();
        float inputWidth = winSize.x * 0.4f;
        float inputHeight = winSize.y * 0.06f;
        float centerX = winSize.x / 2.f;
        float inputYStart = winSize.y * 0.35f;
        float spacing = winSize.y * 0.08f;
        float backSize = winSize.y * 0.05f;
        float backX = winSize.x * 0.02f;
        float backY = winSize.y * 0.02f;

        // Back icon click
        if (engine.getMouseHandler().clickedInRectangle(backX, backY, backSize, backSize)) {
            resetInputFields();
            currentState = MenuState::FriendsMenu;
            return;
        }

        // Input handling
        addFriendBox.handleEvent(event, engine.getMouseHandler());

        // Add button click
        float btnWidth = inputWidth * 0.4f;
        float btnHeight = inputHeight;
        float btnX = centerX - btnWidth / 2.f;
        float btnY = inputYStart + 2.f * spacing;
        if (engine.getMouseHandler().clickedInRectangle(btnX, btnY, btnWidth, btnHeight)) {
            std::string friendName = addFriendBox.getText();
            if (friendName.empty()) {
                addFriendBox.setErrorMessage("Nom d'utilisateur requis");
                return renderAddFriends();
            }
            resetInputFields();
            tetris::GlobalConnection::getInstance().sendRequest(friendName);
            currentState = MenuState::FriendsMenu;
            return;
        }
    }

    if (currentState == MenuState::RemoveFriends) {
        auto winSize = window->getSize();
        float inputWidth = winSize.x * 0.4f;
        float inputHeight = winSize.y * 0.06f;
        float centerX = winSize.x / 2.f;
        float inputYStart = winSize.y * 0.35f;
        float spacing = winSize.y * 0.08f;
        float backSize = winSize.y * 0.05f;
        float backX = winSize.x * 0.02f;
        float backY = winSize.y * 0.02f;

        // Icône retour
        if (engine.getMouseHandler().clickedInRectangle(backX, backY, backSize, backSize)) {
            resetInputFields();
            currentState = MenuState::FriendsMenu;
            return;
        }

        // Gestion de la saisie
        removeFriendBox.handleEvent(event, engine.getMouseHandler());

        // Bouton Supprimer
        float btnWidth = inputWidth * 0.4f;
        float btnHeight = inputHeight;
        float btnX = centerX - btnWidth / 2.f;
        float btnY = inputYStart + 2.f * spacing;
        if (engine.getMouseHandler().clickedInRectangle(btnX, btnY, btnWidth, btnHeight)) {
            std::string friendName = removeFriendBox.getText();
            if (friendName.empty()) {
                removeFriendBox.setErrorMessage("Nom d'utilisateur requis");
                return renderRemoveFriends();
            }
            resetInputFields();
            tetris::GlobalConnection::getInstance().gestion_friend(friendName, false);
            currentState = MenuState::FriendsMenu;
            return;
        }
    }

    if (currentState == MenuState::ManageFriendsRequests) {

        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20)) {
            currentState = MenuState::FriendsMenu;
            return;
        }

        for (size_t i = 0; i < friendRequests.size(); i++) {

            int textY = 200 + (i * 70);
            int yesButtonX = 500, yesButtonY = textY - 10, yesButtonW = 50, yesButtonH = 50;
            int noButtonX = 560, noButtonY = textY - 10, noButtonW = 50, noButtonH = 50;
            if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20)) {
                currentState = MenuState::FriendsMenu;
                return;
            }

            else if (engine.getMouseHandler().clickedInRectangle(yesButtonX, yesButtonY, yesButtonW,
                                                                 yesButtonH)) {
                tetris::GlobalConnection::getInstance().gestion_friend(friendRequests[i], true);
                std::string acceptedFriend = friendRequests[i];
                friendRequests.erase(friendRequests.begin() + i);
                response = acceptedFriend + " a ete ajoute a votre liste d'amis";
                currentState = MenuState::FriendsRequestsResponse;
                return;
            }

            else if (engine.getMouseHandler().clickedInRectangle(noButtonX, noButtonY, noButtonW,
                                                                 noButtonH)) {
                tetris::GlobalConnection::getInstance().gestion_friend(friendRequests[i], false);
                std::string refusedFriend = friendRequests[i];
                friendRequests.erase(friendRequests.begin() + i);
                response = refusedFriend + " a ete refuse";
                currentState = MenuState::FriendsRequestsResponse;
                return;
            }
        }
    }

    if (currentState == MenuState::FriendsRequestsResponse) {

        if (engine.getMouseHandler().clickedInRectangle(320, 200, 150, 50)) {
            currentState = MenuState::ManageFriendsRequests;
            return;
        }
    }

    if (currentState == MenuState::FriendsList) {
        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20)) {
            currentState = MenuState::FriendsMenu;
            return;
        }
    }

    if (currentState == MenuState::TopScores) {
        if (engine.getMouseHandler().clickedInRectangle(10, 10, 20, 20)) {
            currentState = MenuState::GameMenu;
            return;
        }
    }

    if (currentState == MenuState::EndlessMode) {
        if (endlessGame) {
            endlessGame->handleEvents(event);
        }
    }

    if (currentState == MenuState::ClassicGameActive) {
        if (classicGame) {
            classicGame->handleEvents(event);

            // Check if game wants to return to menu
            if (classicGame->wantsToReturnToMenu()) {
                classicGame.reset();
                room.leaveRoom();
                currentState = MenuState::MultiPlayerMode;
            }
        }
    }

    if (currentState == MenuState::DuelModeActive) {
        if (duelGame) {
            duelGame->handleEvents(event);

            // Check if game wants to return to menu
            if (duelGame->wantsToReturnToMenu()) {
                duelGame.reset();
                room.leaveRoom();
                currentState = MenuState::MultiPlayerMode;
            }
        }
    }

    if (currentState == MenuState::RoyalCompetitionActive) {
        if (royalCompetitionGame) {
            royalCompetitionGame->handleEvents(event);

            // Check if game wants to return to menu
            if (royalCompetitionGame->wantsToReturnToMenu()) {
                royalCompetitionGame.reset();
                room.leaveRoom();
                currentState = MenuState::MultiPlayerMode;
            }
        }
    }
}

bool Menu::handleEyeClick(bool &showPassword, InputHandler &passwordBox, int x, int y) {
    if (engine.getMouseHandler().clickedInRectangle(x, y, 40, 40)) {
        showPassword = !showPassword;
        passwordBox.setPasswordHidden(!showPassword);
        return true;
    }
    return false;
}

bool Menu::wantsToStartGame() const {
    return startGame;
}

void Menu::setWantsToStartGame(bool wants) {
    startGame = wants;
}

void Menu::renderMessaging() {
    engine.drawBackground();
    engine.drawText("Messagerie", 320, 120, 32, sf::Color::White);
    engine.drawImage(ASSETS_SRC "/menu/back.png", 10, 10, 20, 20);

    sendMessageToBox.render(engine);
    engine.drawText("Destinataire :", 100, 170, 20, sf::Color::White);

    sendMessageContentBox.render(engine);
    engine.drawText("Message :", 100, 240, 20, sf::Color::White);

    engine.drawRectangle(100, 330, 100, 40, sf::Color(50, 200, 50));
    engine.drawText("Envoyer", 110, 340, 20, sf::Color::White);
}
