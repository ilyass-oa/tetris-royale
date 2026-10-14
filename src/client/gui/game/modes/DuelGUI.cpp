#include "DuelGUI.hpp"
#include "../../../network/GlobalConnexion.hpp"
#include "../../engine/SFMLEngine.hpp"
#include "ClassicGUI.hpp"
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

// Mutex séparés pour éviter les blocages entre threads
std::mutex grid_mutex;       // Protection de la grille principale
std::mutex player_mutex;     // Protection des données des autres joueurs
std::mutex game_state_mutex; // Protection des données d'état du jeu

// Constantes pour la configuration
constexpr int EMPTY_COLOR = 8;
constexpr int PING_INTERVAL_MS = 250; // Intervalle de ping
constexpr int NETWORK_SLEEP_MS = 3;   // Temps de sommeil du thread réseau

DuelGUI::DuelGUI(tetris::gui::SFMLEngine &engine, sf::RenderWindow *window)
    : window(window), engine(engine) {
    // Create a ClassicGUI instance that will handle all the actual functionality
    classicMode = std::make_unique<ClassicGUI>(engine, window);
}

DuelGUI::~DuelGUI() {
    stop();
}

void DuelGUI::start() {
    // Just start the classic mode
    classicMode->start();
}

void DuelGUI::stop() {
    if (classicMode) {
        classicMode->stop();
    }
}

void DuelGUI::update(float deltaTime) {
    if (classicMode) {
        classicMode->update(deltaTime);
    }
}

void DuelGUI::handleEvents(const sf::Event &event) {
    if (classicMode) {
        classicMode->handleEvents(event);
    }
}

void DuelGUI::render() {
    if (classicMode) {
        classicMode->render();
    }
}

bool DuelGUI::wantsToReturnToMenu() const {
    if (classicMode) {
        return classicMode->wantsToReturnToMenu();
    }
    return true;
}

void DuelGUI::handlePlayerElimination(int playerIndex) {
    if (classicMode) {
        classicMode->handlePlayerElimination(playerIndex);
    }
}

void DuelGUI::refresh_next_piece(tetris::PieceType newPiece) {
    if (classicMode) {
        // classicMode->refresh_next_piece(newPiece);
    }
}

} // namespace tetris::gui