#ifndef ROYALCOMPETITIONGUI_HPP
#define ROYALCOMPETITIONGUI_HPP

#include "../GameInputHandler.hpp"
#include "../GameInterface.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include <array>
#include <atomic>
#include <game/Malus.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace tetris {
class Packet;
enum PieceType;
class TetrisGame;

namespace gui {
class SFMLEngine;
}
} // namespace tetris

namespace tetris::gui {

// Enum pour les types de malus
enum class MalusType {
    InvertControls,
    BlockControls,
    SpeedFall,
    Lightning2x2,
    ScreenBlackout,
    Blocks1x1
};

// Structure pour les informations de malus
struct MalusInfo {
    std::string name;
    int cost;
    MalusType type;
};

class RoyalCompetitionGUI final {
  public:
    RoyalCompetitionGUI(SFMLEngine &engine, sf::RenderWindow *window);
    ~RoyalCompetitionGUI();

    void start();
    void stop();
    void update(float deltaTime);
    void handleEvents(const sf::Event &event);
    void render();

    [[nodiscard]] bool wantsToReturnToMenu() const;

    // Pour les futures implémentations
    void sendMalus(int lines);
    void receiveMalus(int lines);
    void handlePlayerElimination(int playerIndex);
    void refresh_grid(int playerIndex, const std::vector<std::vector<int>> &grid);
    void refresh_energy(int newEnergyLevel);
    void refresh_next_piece(tetris::PieceType newPiece);

  private:
    /* --- constantes locales (évite les conflits) ------------------------ */
    static constexpr int GRID_WIDTH = 10;
    static constexpr int GRID_HEIGHT = 20;
    static constexpr int EMPTY_COLOR = 8;
    static constexpr int RED_COLOR = 4;   // Code couleur pour le rouge (joueur mort)
    static constexpr int MAX_PLAYERS = 8; // hors joueur local

    // Constantes pour le menu des malus
    static constexpr int MALUS_MENU_WIDTH = 200;
    static constexpr int MALUS_MENU_HEIGHT = 250;
    static constexpr int MALUS_MENU_ITEM_HEIGHT = 30;
    static constexpr int MALUS_MENU_PADDING = 10;

    /* --- structures internes -------------------------------------------- */
    using Grid = std::array<std::array<int, GRID_WIDTH>, GRID_HEIGHT>;

    struct PlayerGameState {
        std::vector<std::vector<int>> grid;
        bool active{false};
        bool eliminated{false}; // Indique si le joueur est éliminé
    };

    // Structure pour stocker les informations de malus sélectionné
    struct MalusSelection {
        int sourcePlayerId;
        int targetPlayerId;
        MalusType malusType;
    };

    /* --- réseau --------------------------------------------------------- */
    void networkLoop();
    void processPacket(std::shared_ptr<tetris::Packet> packet);
    void updatePlayerState(int playerIndex, const std::vector<std::vector<int>> &newGrid);
    tetris::Malus convertMalusTypeToServerEnum(MalusType type);
    void sendMalusToServer(int targetPlayerId, MalusType malusType);

    /* --- rendu & utilitaires ------------------------------------------- */
    bool allCellsEmpty(const std::vector<std::vector<int>> &grid);
    bool isPlayerEliminated(const std::vector<std::vector<int>> &grid);
    void renderOtherPlayers(int mainGridX, int mainGridY, float mainWidth, float mainHeight,
                            float windowWidth, float windowHeight);

    /* --- fonctions pour le menu de malus ------------------------------- */
    void initMalusMenu();
    void renderMalusMenu();
    bool checkPlayerGridClicked(int playerId, float x, float y, float width, float height);
    bool hasEnoughEnergy(int malusCost) const;
    void storeMalusSelection(int targetPlayerId, MalusType malusType);

    /* --- interface de jeu ----------------------------------------------- */
    GameInterface gameInterface;
    std::unique_ptr<GameInputHandler> inputHandler;

    /* --- mutex pour la synchronisation ---------------------------------- */
    mutable std::mutex grid_mutex;       // Protection de la grille principale
    mutable std::mutex player_mutex;     // Protection des données des autres joueurs
    mutable std::mutex game_state_mutex; // Protection des données d'état du jeu
    mutable std::mutex malus_mutex;      // Protection des données de malus

    /* --- état ----------------------------------------------------------- */
    SFMLEngine &engine;
    sf::RenderWindow *window;

    std::vector<std::vector<int>> gridData;
    std::map<int, std::vector<std::vector<int>>> otherPlayersGrids;
    std::map<int, bool> otherPlayersStatus; // true = actif, false = éliminé
    std::map<int, PlayerGameState> playerStates;
    std::unordered_set<sf::Keyboard::Key> pressedKeys;

    std::string roomName;
    std::string malus;
    std::atomic<int> playerCount{0};
    std::atomic<int> energy{0};
    std::atomic<tetris::PieceType> nextPiece{};

    std::atomic_bool running{false};
    std::atomic_bool gameOver{false};

    bool isOwner{false};

    /* --- état du menu de malus ----------------------------------------- */
    bool malusMenuOpen{false};
    int targetedPlayerId{-1};
    std::vector<MalusInfo> availableMaluses;
    std::vector<MalusSelection> malusSelections;
    sf::Vector2f malusMenuPosition{0.0f, 0.0f};
};

} // namespace tetris::gui

#endif // ROYALCOMPETITIONGUI_HPP
