#ifndef CONNEXION_HPP
#define CONNEXION_HPP

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <packets_include.hpp>

// For UDP Discovery Client
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace tetris {

class Client {
    int s_socket{-1};
    int player_number{0};
    bool flag = true;
    std::string current_room;
    int player_count = 0;
    bool is_owner = false;
    std::string server_address;
    unsigned short server_port;
    bool connected{false};
    bool authenticated{false};
    mutable std::mutex state_mutex;
    std::mutex io_mutex;
    std::string last_error;

    bool open_connection();
    bool ensure_connected();
    void disconnect(const std::string &reason = {});
    void set_error(const std::string &error);
    bool send_packet(const Packet &packet);
    bool authenticate(const std::string &name, const std::string &password, bool create_account);

  public:
    explicit Client(const std::string &adresse_serveur);
    ~Client();
    Client(const Client &) = delete;
    Client &operator=(const Client &) = delete;

    bool is_connected() const;
    bool isConnected() const {
        return is_connected();
    }
    std::string get_last_error() const;
    std::string getLastError() const {
        return get_last_error();
    }

    // flag pour sortir de la reception
    bool get_flag(void);
    void switch_flag(void);

    // Définir le mode du socket (bloquant ou non-bloquant)
    void setNonBlocking(bool nonBlocking);

    // envois specifique
    bool connect_create(std::string name, std::string password);
    bool connect_log(std::string name, std::string password);
    void sendRequest(std::string friendName);
    void gestion_friend(std::string friendName, bool accepted);
    std::vector<std::string> handle_my_requests();

    // recepteur de packet
    std::shared_ptr<Packet> recepteur(void);

    // Accès au socket pour timeout personnalisé
    int get_socket(void) const;
    int getSocket(void) const {
        return get_socket();
    }

    // ce qui envois les données
    void exit(void);
    void send_message(std::string who_recv, std::string message);
    void send_grid(std::vector<std::vector<int>> grid);
    void create_room(int limit, std::string room_name, GameMode game_mode);
    void join_room(std::string room_name);
    void send_message_room(std::string room_name, std::string message);
    void change_status(GameMode quoi);
    void start_game(GameMode game_mode);
    void send_input(int input);
    void send_malus(Malus effect, int target);
    void send_score(int score);
    void end_game(bool end);
    int request_score();
    std::vector<std::string> friends_list();
    std::vector<std::string> top_scores_list();
    void set_player_number(int player_number);
    int get_player_number(void);

    // Room information methods
    std::string get_current_room() const {
        std::lock_guard<std::mutex> lock(state_mutex);
        return current_room;
    }
    int get_player_count() const {
        std::lock_guard<std::mutex> lock(state_mutex);
        return player_count;
    }
    bool is_room_owner() const {
        std::lock_guard<std::mutex> lock(state_mutex);
        return is_owner;
    }

    // Method for LAN server discovery
    static std::string decouvrirServeurLAN(unsigned short discovery_port, int timeout_ms);
};
} // namespace tetris

#endif
