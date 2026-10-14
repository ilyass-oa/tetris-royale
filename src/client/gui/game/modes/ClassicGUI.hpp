#ifndef CLASSICGUI_HPP
#define CLASSICGUI_HPP

#include "../GameInputHandler.hpp"
#include "../GameInterface.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include <array>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace tetris {
class Packet;     // fwd
enum PieceType;   // unscoped enum forward declaration
class TetrisGame; // fwd

namespace gui {
class SFMLEngine;
} // namespace gui
} // namespace tetris

namespace tetris::gui {

class ClassicGUI final {
  public:
    ClassicGUI(SFMLEngine &engine, sf::RenderWindow *window);
    ~ClassicGUI();

    void start();
    void stop();
    void update(float deltaTime);
    void handleEvents(const sf::Event &event);
    void render();

    [[nodiscard]] bool wantsToReturnToMenu() const;

    // Pour les futures implémentations
    void handlePlayerElimination(int playerIndex);
    void refresh_next_piece(tetris::PieceType newPiece);

  private:
    /* --- constantes locales (évite les conflits) ------------------------ */
    static constexpr int GRID_WIDTH = 10;
    static constexpr int GRID_HEIGHT = 20;
    static constexpr int EMPTY_COLOR = 8;
    static constexpr int MAX_PLAYERS = 8; // hors joueur local

    /* --- structures internes -------------------------------------------- */
    using Grid = std::array<std::array<int, GRID_WIDTH>, GRID_HEIGHT>;

    struct PlayerGameState {
        std::vector<std::vector<int>> grid;
        bool active{false};
    };

    /* --- réseau --------------------------------------------------------- */
    void networkLoop();
    void processPacket(std::shared_ptr<tetris::Packet> packet);
    void updatePlayerState(int playerIndex, const std::vector<std::vector<int>> &newGrid);

    /* --- rendu & utilitaires ------------------------------------------- */
    bool allCellsEmpty(const std::vector<std::vector<int>> &grid);
    void renderOtherPlayers(int mainGridX, int mainGridY, float mainWidth, float mainHeight,
                            float windowWidth, float windowHeight);

    /* --- interface de jeu ----------------------------------------------- */
    GameInterface gameInterface;
    std::unique_ptr<GameInputHandler> inputHandler;

    /* --- mutex pour la synchronisation ---------------------------------- */
    std::mutex grid_mutex;       // Protection de la grille principale
    std::mutex player_mutex;     // Protection des données des autres joueurs
    std::mutex game_state_mutex; // Protection des données d'état du jeu

    /* --- état ----------------------------------------------------------- */
    SFMLEngine &engine;
    sf::RenderWindow *window;

    std::vector<std::vector<int>> gridData;
    std::map<int, std::vector<std::vector<int>>> otherPlayersGrids;
    std::map<int, bool> otherPlayersStatus; // true = actif, false = éliminé
    std::map<int, PlayerGameState> playerStates;
    std::unordered_set<sf::Keyboard::Key> pressedKeys;

    std::string roomName;
    std::atomic<int> playerCount{0};
    std::atomic<tetris::PieceType> nextPiece{};

    std::atomic_bool running{false};
    std::atomic_bool gameOver{false};

    int score{0};
    bool isOwner{false};
};

} // namespace tetris::gui

#endif // CLASSICGUI_HPP
