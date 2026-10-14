#pragma once
#include "player.hpp"
#include <atomic>
#include <list>
#include <memory>
#include <thread>
namespace tetris::serveur {
struct ServerConfig {
    unsigned short port = 46587, discovery_port = 46588;
    std::string bind_address = "127.0.0.1";
    std::string database_path = "data/Users.json";
};
class Serveur {
    ServerConfig config;
    int listener = -1, discovery = -1;
    std::shared_ptr<PlayersManager> players;
    std::shared_ptr<RoomsManager> rooms;
    std::shared_ptr<DatabaseManager> data;
    struct Session {
        std::shared_ptr<Player> player;
        std::jthread thread;
    };
    std::list<Session> sessions;
    void answer_discovery();

  public:
    explicit Serveur(ServerConfig config = {});
    ~Serveur();
    void run(const std::atomic<bool> &stop);
    void power_off();
};
} // namespace tetris::serveur
