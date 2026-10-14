#pragma once
#include "../database/database_manager.hpp"
#include "player_manager.hpp"
#include "room_manager.hpp"
#include <atomic>
#include <deque>
#include <game/Malus.hpp>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <packets_include.hpp>
#include <string>
#include <sys/socket.h>
#include <tuple>
namespace tetris::serveur {
class Player : public std::enable_shared_from_this<Player> {
    const int socket_fd;
    std::atomic<bool> quit{false}, cleaned{false}, done{false};
    std::atomic<GameMode> current_mode{GameMode::Menu};
    mutable std::mutex state_mutex;
    std::mutex send_mutex, input_mutex;
    std::string name, current_room;
    std::deque<int> inputs;
    std::deque<std::tuple<int, Malus>> attacks;
    std::shared_ptr<PlayersManager> players;
    std::shared_ptr<RoomsManager> rooms;
    std::shared_ptr<DatabaseManager> data;
    bool authenticate();
    void handle(const std::shared_ptr<Packet> &packet);
    void leave_room();
    void publish_room();
    void create_room(int limit, const std::string &name, GameMode mode);
    void join_room(const std::string &name);
    void start_game(GameMode mode);

  public:
    Player(int fd, sockaddr_in address, std::shared_ptr<PlayersManager> players,
           std::shared_ptr<RoomsManager> rooms, std::shared_ptr<DatabaseManager> data);
    ~Player();
    std::string get_name() const;
    std::string get_current_room() const;
    int get_socket() const {
        return socket_fd;
    }
    bool is_finished() const {
        return done.load();
    }
    bool is_connected() const {
        return !quit.load();
    }
    bool is_playing_in(const std::string &room) const;
    void change_status(GameMode mode) {
        current_mode.store(mode);
    }
    void player_thread();
    void exit();
    void input_purge();
    void input_push(int input);
    int input_pop();
    std::tuple<int, Malus> malus_to_other_pop();
    void save_score(int score) {
        data->saveScore(get_name(), score);
    }
    template <class T, class... Args> void send_packet(Args &&...args) {
        std::lock_guard lock(send_mutex);
        if (quit.load())
            throw std::runtime_error("Session closed");
        try {
            T(std::forward<Args>(args)...).send(socket_fd);
        } catch (...) {
            quit.store(true);
            ::shutdown(socket_fd, SHUT_RDWR);
            throw;
        }
    }
};
} // namespace tetris::serveur
