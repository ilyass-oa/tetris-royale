#ifndef MENU_HPP
#define MENU_HPP

#include "../../network/GlobalConnexion.hpp"
#include "../../network/connexion.hpp"
#include "../engine/InputHandler.hpp"
#include "../engine/SFMLEngine.hpp"
#include "../game/modes/ClassicGUI.hpp"
#include "../game/modes/DuelGUI.hpp"
#include "../game/modes/EndlessGUI.hpp"
#include "../game/modes/RoyalCompetGUI.hpp"
#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

#include "Room.hpp"
#include <atomic>
#include <future>
#include <mutex>
#include <thread>

enum class MenuState {
    MainMenu,
    LoginInput,
    RegisterInput,
    SoloMode,
    MultiPlayerMode,
    RoomWaiting,
    Discussion,
    CreateRoom,
    CreateRoomResponse,
    JoinRoom,
    JoinRoomResponse,
    HostStartGame,
    ClassiqueMode,
    DuelMode,
    RoyalCompetitionMode,
    FriendsMenu,
    AddFriends,
    RemoveFriends,
    ManageFriendsRequests,
    FriendsRequestsResponse,
    FriendsList,
    TopScores,
    GameMenu,
    GameModeMenu,
    QuitConfirmation,
    QUIT,
    EndlessMode,
    ClassicGameActive,
    DuelModeActive,
    RoyalCompetitionActive,
    Messaging,
};

class Menu {
  public:
    Menu(tetris::gui::SFMLEngine &engine, sf::RenderWindow *window);
    ~Menu();

    void update(float deltaTime);
    void render();
    void handleEvents(const sf::Event &event);
    bool wantsToStartGame() const;
    void setWantsToStartGame(bool value);
    bool showLoginPassword = false;
    bool showRegisterPassword = false;
    bool handleAuthEvent;
    std::string hote;

  private:
    tetris::gui::SFMLEngine &engine;
    sf::RenderWindow *window;

    MenuState currentState = MenuState::MainMenu;

    // Game modes
    std::unique_ptr<tetris::gui::EndlessGUI> endlessGame;
    std::unique_ptr<tetris::gui::ClassicGUI> classicGame;
    std::unique_ptr<tetris::gui::DuelGUI> duelGame;
    std::unique_ptr<tetris::gui::RoyalCompetitionGUI> royalCompetitionGame;

    // Nouvelle instance de Room pour la gestion des rooms
    tetris::gui::Room room;

    // Room state variables
    std::string current_room = "";
    int player_in_room = 0;
    tetris::GameMode mode = tetris::GameMode::Menu;
    int maxPlayer = 9;
    bool startGame = false;

    // Champs de saisie (username/password)
    std::string user;
    std::string pass;
    InputHandler loginUsernameBox;
    InputHandler loginPasswordBox;
    InputHandler registerUsernameBox;
    InputHandler registerPasswordBox;
    InputHandler addFriendBox;
    InputHandler removeFriendBox;
    InputHandler createRoomBox;
    InputHandler joinRoomBox;
    InputHandler sendMessageToBox;
    InputHandler sendMessageContentBox;
    InputHandler roomChatBox;

    // Other state variables
    std::vector<std::string> friendRequests;
    std::string response;
    std::vector<std::string> friendsList;
    std::vector<std::string> topScores;
    std::string selectedFriendRequest;
    std::string messageRoom;
    std::vector<std::string> messages;

    // Méthodes privées
    void renderButton(const std::string text, std::vector<std::string> buttonTexts);
    void resetInputFields();
    void renderMainMenu();
    void renderLoginInput();
    void renderRegisterInput();
    void renderQuitConfirmation();

    void renderGameMenu();
    void renderGameMode();

    void renderSoloMode();
    void renderEndlessMode();            // Endless Mode method
    void renderClassicGameActive();      // Classic game rendering method
    void renderDuelModeActive();         // Duel game rendering method
    void renderRoyalCompetitionActive(); // Royal Competition game rendering method

    void renderMultiPlayerMode();

    void room_waiting(tetris::GameMode mode);
    void hostStartGame();
    void renderCreateRoom();
    void renderCreateRoomResponse();
    void renderJoinRoom();
    void renderJoinRoomResponse();
    void renderDiscussion();

    void renderFriendsMenu();
    void renderAddFriends();
    void renderRemoveFriends();
    void renderFriendList();
    void renderManageFriendsRequests();
    void renderFriendsRequestsResponse();

    void renderTopScores();

    void renderMessaging();

    bool handleEyeClick(bool &showPassword, InputHandler &passwordBox, int x, int y);
};

#endif // MENU_HPP
