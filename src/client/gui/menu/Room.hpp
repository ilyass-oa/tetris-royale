#pragma once

#include "../../network/GlobalConnexion.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <packets_include.hpp>
#include <string>
#include <vector>

namespace tetris {
namespace gui {

class Room {
  public:
    Room();
    ~Room();

    bool createRoom(const std::string &name, tetris::GameMode mode);
    bool joinRoom(const std::string &name);
    bool startGame(tetris::GameMode mode);
    bool shouldStartGame() const;
    void resetStartGameFlag();

    tetris::GameMode getGameMode() const;
    void setGameMode(tetris::GameMode mode);
    std::string getRoomName() const;
    int getPlayerCount() const;

    bool isPlayerCountValid() const;

    void setHost(const std::string &hostName);

    std::string getHost() const;

    bool isHost(const std::string &userName) const;

    void update();

    void leaveRoom();

    void setOnRoomUpdateCallback(std::function<void()> callback);

    const std::vector<std::string> &getMessages() const;

  private:
    std::string name;                ///< Nom de la room
    std::string host;                ///< Nom de l'hôte de la room
    int playerCount;                 ///< Nombre de joueurs dans la room
    tetris::GameMode gameMode;       ///< Mode de jeu de la room
    std::atomic<bool> gameStartFlag; ///< Flag indiquant si le jeu doit démarrer

    std::atomic<bool> polling;

    std::function<void()> onRoomUpdate; ///< Callback appelé lors d'une mise à jour de la room

    std::string messageRoom;
    std::vector<std::string> messages;

    void startPolling();

    void stopPolling();

    void handlePacket(std::shared_ptr<tetris::Packet> packet);
};

} // namespace gui
} // namespace tetris
