#include "Room.hpp"
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <system_error>
#include <unistd.h>

namespace tetris {
namespace gui {

// Constantes pour les modes de jeu
const int CLASSIQUE_MIN_PLAYERS = 2;
const int CLASSIQUE_MAX_PLAYERS = 9;
const int DUEL_MIN_PLAYERS = 2;
const int DUEL_MAX_PLAYERS = 2;
const int BR_MIN_PLAYERS = 2;
const int BR_MAX_PLAYERS = 9;

Room::Room()
    : name(""), host(""), playerCount(0), gameMode(tetris::GameMode::Menu), gameStartFlag(false),
      polling(false), onRoomUpdate(nullptr) {
    // Rien d'autre à initialiser ici
}

Room::~Room() {
    stopPolling();
}

bool Room::createRoom(const std::string &roomNameInput, tetris::GameMode modeInput) {
    stopPolling();

    // Réinitialiser l'état / initialiser avec les valeurs attendues
    this->name = "";       // Sera confirmé par le serveur
    this->playerCount = 0; // Sera confirmé par le serveur
    this->gameMode = modeInput;
    this->host = ""; // Sera défini par Menu.cpp ou confirmé par le serveur
    this->gameStartFlag.store(false);

    // Vérifier les limites de joueurs pour ce mode
    int maxPlayers;

    if (modeInput == tetris::GameMode::Duel) {
        maxPlayers = DUEL_MAX_PLAYERS;
    } else if (modeInput == tetris::GameMode::Classique) {
        maxPlayers = CLASSIQUE_MAX_PLAYERS;
    } else if (modeInput == tetris::GameMode::BR) {
        maxPlayers = BR_MAX_PLAYERS;
    } else {
        std::cerr << "Mode de jeu non supporté pour la création de room" << std::endl;
        return false; // Échec initial, mode invalide
    }

    // Envoyer la demande de création au serveur
    tetris::GlobalConnection::getInstance().create_room(maxPlayers, roomNameInput, modeInput);

    // Définir le nom de manière optimiste pour l'affichage initial
    // Le nom réel et le nombre de joueurs seront confirmés par un paquet RoomInfos.
    this->name = roomNameInput;
    // Le créateur est implicitement le premier joueur, mais le playerCount sera mis à jour par
    // RoomInfos. On pourrait mettre playerCount = 1 ici, mais laissons RoomInfos être la source de
    // vérité.

    startPolling();

    // Forcer une mise à jour pour que l'UI reflète l'état "en cours de création"
    update();

    // Retourne true pour indiquer que la tentative de création a été initiée.
    // Le succès réel est confirmé par le réseau.
    return true;
}

bool Room::joinRoom(const std::string &roomNameToJoin) {
    stopPolling();

    // Réinitialiser l'état
    this->name = ""; // Sera mis à jour par RoomInfos
    this->playerCount = 0;
    // gameMode et host seront également mis à jour par RoomInfos si le serveur les envoie.
    this->gameStartFlag.store(false);

    // Essayer de rejoindre la room (la méthode est void)
    tetris::GlobalConnection::getInstance().join_room(roomNameToJoin);

    // Définir un nom temporaire pour l'affichage pendant la tentative de connexion
    this->name = "Tentative de connexion à " + roomNameToJoin + "...";

    startPolling();

    // Forcer une mise à jour pour que l'UI reflète l'état "en cours de connexion"
    update();

    // Retourne true pour indiquer que la tentative de connexion a été initiée.
    // Le succès/échec est confirmé par le réseau.
    // Les sleeps précédents ont été retirés car ils ne garantissent rien.
    return true;
}

bool Room::startGame(tetris::GameMode mode) {
    // Vérifier que le nombre de joueurs est valide pour ce mode
    if (!isPlayerCountValid()) {
        std::cerr << "Nombre de joueurs invalide pour démarrer le jeu" << std::endl;
        return false;
    }

    // Envoyer la demande de démarrage au serveur (la méthode est void)
    tetris::GlobalConnection::getInstance().start_game(mode);

    return true;
}

bool Room::shouldStartGame() const {
    return gameStartFlag.load();
}

void Room::resetStartGameFlag() {
    gameStartFlag.store(false);
}

tetris::GameMode Room::getGameMode() const {
    return gameMode;
}

void Room::setGameMode(tetris::GameMode mode) {
    gameMode = mode;
}

std::string Room::getRoomName() const {
    return name;
}

int Room::getPlayerCount() const {
    return playerCount;
}

bool Room::isPlayerCountValid() const {
    int minPlayers = 0;
    int maxPlayers = 0;

    switch (gameMode) {
    case tetris::GameMode::Classique:
        minPlayers = CLASSIQUE_MIN_PLAYERS;
        maxPlayers = CLASSIQUE_MAX_PLAYERS;
        break;
    case tetris::GameMode::Duel:
        minPlayers = DUEL_MIN_PLAYERS;
        maxPlayers = DUEL_MAX_PLAYERS;
        break;
    case tetris::GameMode::BR:
        minPlayers = BR_MIN_PLAYERS;
        maxPlayers = BR_MAX_PLAYERS;
        break;
    default:
        return false;
    }

    return playerCount >= minPlayers && playerCount <= maxPlayers;
}

void Room::setHost(const std::string &hostName) {
    host = hostName;
}

std::string Room::getHost() const {
    return host;
}

bool Room::isHost(const std::string &userName) const {
    return userName == host;
}

void Room::update() {
    if (polling.load()) {
        for (int i = 0; i < 32 && polling.load(); ++i) {
            auto packet = tetris::GlobalConnection::getInstance().recepteur();
            if (!packet)
                break;
            handlePacket(packet);
        }
    }
}

void Room::leaveRoom() {
    stopPolling();

    // Quitter la room côté serveur (à implémenter dans GlobalConnection et décommenter)
    try {
        tetris::GlobalConnection::getInstance().change_status(tetris::GameMode::Menu);
    } catch (const std::exception &) {
    }

    // Réinitialiser l'état
    name = "";
    host = "";
    playerCount = 0;
    gameStartFlag.store(false);

    // Notifier l'UI du changement d'état
    update();
}

void Room::setOnRoomUpdateCallback(std::function<void()> callback) {
    onRoomUpdate = callback;
}

void Room::startPolling() {
    polling.store(true);
    gameStartFlag.store(false);
}
void Room::stopPolling() {
    polling.store(false);
}

void Room::handlePacket(std::shared_ptr<tetris::Packet> packet) {
    // Traiter le paquet en fonction de son type
    if (auto roomInfo = std::dynamic_pointer_cast<tetris::clientbound_packets::RoomInfos>(packet)) {

        if (roomInfo->reponse) {
            gameMode = roomInfo->mode;
            host = roomInfo->owner;
            name = roomInfo->room_name;
            playerCount = roomInfo->numb_player;

            if (onRoomUpdate) {
                onRoomUpdate();
            }
        } else {
            name = "";
            playerCount = 0;
            if (onRoomUpdate) {
                onRoomUpdate();
            }
        }
    } else if (auto startPacket =
                   std::dynamic_pointer_cast<tetris::clientbound_packets::GameStart>(packet)) {
        if (!gameStartFlag.load()) {
            gameStartFlag.store(true);
            stopPolling(); // Arrêt après GameStart comme avant
        }
    } else if (auto message =
                   std::dynamic_pointer_cast<tetris::clientbound_packets::Message>(packet)) {
        messages.push_back(message->texte);
        if (messages.size() > 18) {
            messages.erase(messages.begin());
        }
        if (onRoomUpdate) {
            onRoomUpdate();
        }
    } else {
        if (packet) {
            // If dynamic_pointer_cast failed for all known types, it's an unknown type.
        } else {
        }
    }
}

const std::vector<std::string> &Room::getMessages() const {
    return messages;
}

} // namespace gui
} // namespace tetris
