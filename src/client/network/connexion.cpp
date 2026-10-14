#include "connexion.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <packet_read.hpp>
#include <poll.h>
#include <shared_constante.hpp>
#include <sys/socket.h>
#include <transport.hpp>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>

namespace {

constexpr unsigned short kDefaultDiscoveryPort = 46588;
constexpr char kDiscoveryRequest[] = "TETRIS_DISCOVER_LAN_V1";
constexpr char kDiscoveryResponse[] = "TETRIS_SERVER_LAN_V1";
constexpr int kDefaultDiscoveryTimeoutMs = 3000;
constexpr int kDefaultConnectTimeoutMs = 3000;

unsigned short port_from_environment() {
    const char *value = std::getenv("TETRIS_PORT");
    if (value == nullptr || *value == '\0') {
        return static_cast<unsigned short>(PORT);
    }

    char *end = nullptr;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (end == value || *end != '\0' || parsed == 0 || parsed > 65535UL) {
        return static_cast<unsigned short>(PORT);
    }
    return static_cast<unsigned short>(parsed);
}

int connect_timeout_from_environment() {
    const char *value = std::getenv("TETRIS_CONNECT_TIMEOUT_MS");
    if (value == nullptr || *value == '\0') {
        return kDefaultConnectTimeoutMs;
    }
    char *end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed <= 0 || parsed > 60000L) {
        return kDefaultConnectTimeoutMs;
    }
    return static_cast<int>(parsed);
}

bool set_close_on_exec(int descriptor) {
    const int flags = ::fcntl(descriptor, F_GETFD, 0);
    return flags >= 0 && ::fcntl(descriptor, F_SETFD, flags | FD_CLOEXEC) == 0;
}

bool set_nonblocking_fd(int descriptor, bool nonblocking) {
    const int flags = ::fcntl(descriptor, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    const int new_flags = nonblocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return ::fcntl(descriptor, F_SETFL, new_flags) == 0;
}

} // namespace

namespace tetris {

Client::Client(const std::string &adresse_serveur)
    : server_address(adresse_serveur.empty() && std::getenv("TETRIS_SERVER") != nullptr
                         ? std::getenv("TETRIS_SERVER")
                         : adresse_serveur),
      server_port(port_from_environment()) {
    if (server_address.empty()) {
        server_address = "127.0.0.1";
    }
    // A failed initial connect leaves a usable object.  Authentication calls
    // retry through ensure_connected(), which matters when the server starts
    // after the client UI.
    open_connection();
}

Client::~Client() {
    disconnect();
}

bool Client::is_connected() const {
    std::lock_guard<std::mutex> lock(state_mutex);
    return connected;
}

std::string Client::get_last_error() const {
    std::lock_guard<std::mutex> lock(state_mutex);
    return last_error;
}

int Client::get_socket(void) const {
    std::lock_guard<std::mutex> lock(state_mutex);
    return s_socket;
}

void Client::set_error(const std::string &error) {
    std::lock_guard<std::mutex> lock(state_mutex);
    last_error = error;
}

bool Client::open_connection() {
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        if (connected && s_socket >= 0) {
            return true;
        }
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(server_port);
    if (::inet_pton(AF_INET, server_address.c_str(), &address.sin_addr) != 1) {
        set_error("invalid server address: " + server_address);
        return false;
    }

    const int descriptor = ::socket(AF_INET, SOCK_STREAM, 0);
    if (descriptor < 0) {
        set_error(std::string("socket creation failed: ") + std::strerror(errno));
        return false;
    }
    set_close_on_exec(descriptor);

    if (!set_nonblocking_fd(descriptor, true)) {
        set_error(std::string("failed to configure socket: ") + std::strerror(errno));
        ::close(descriptor);
        return false;
    }

    int result = ::connect(descriptor, reinterpret_cast<sockaddr *>(&address), sizeof(address));
    if (result < 0 && errno != EINPROGRESS) {
        set_error(std::string("connection failed: ") + std::strerror(errno));
        ::close(descriptor);
        return false;
    }

    if (result < 0) {
        struct pollfd descriptor_poll{descriptor, POLLOUT, 0};
        const int timeout_ms = connect_timeout_from_environment();
        do {
            result = ::poll(&descriptor_poll, 1, timeout_ms);
        } while (result < 0 && errno == EINTR);

        if (result == 0) {
            set_error("connection timed out");
            ::close(descriptor);
            return false;
        }
        if (result < 0) {
            set_error(std::string("connection poll failed: ") + std::strerror(errno));
            ::close(descriptor);
            return false;
        }

        int socket_error = 0;
        socklen_t error_size = sizeof(socket_error);
        if (::getsockopt(descriptor, SOL_SOCKET, SO_ERROR, &socket_error, &error_size) < 0 ||
            socket_error != 0) {
            set_error(std::string("connection failed: ") +
                      std::strerror(socket_error == 0 ? errno : socket_error));
            ::close(descriptor);
            return false;
        }
    }

    if (!set_nonblocking_fd(descriptor, false)) {
        set_error(std::string("failed to restore blocking mode: ") + std::strerror(errno));
        ::close(descriptor);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(state_mutex);
        s_socket = descriptor;
        connected = true;
        authenticated = false;
        last_error.clear();
    }
    return true;
}

bool Client::ensure_connected() {
    if (is_connected()) {
        return true;
    }
    return open_connection();
}

void Client::disconnect(const std::string &reason) {
    int descriptor = -1;
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        descriptor = s_socket;
        s_socket = -1;
        connected = false;
        authenticated = false;
        current_room.clear();
        player_count = 0;
        is_owner = false;
        if (!reason.empty()) {
            last_error = reason;
        }
    }

    if (descriptor >= 0) {
        transport::forget_socket(descriptor);
        ::shutdown(descriptor, SHUT_RDWR);
        ::close(descriptor);
    }
}

bool Client::send_packet(const Packet &packet) {
    std::lock_guard<std::mutex> io_lock(io_mutex);
    const int descriptor = get_socket();
    if (descriptor < 0 || !is_connected()) {
        set_error("cannot send while disconnected");
        return false;
    }

    try {
        packet.send(descriptor);
        return true;
    } catch (const std::exception &error) {
        disconnect(error.what());
        return false;
    }
}

bool Client::authenticate(const std::string &name, const std::string &password,
                          bool create_account) {
    std::lock_guard<std::mutex> io_lock(io_mutex);

    if (is_connected() && authenticated) {
        disconnect("reconnecting for a new authentication attempt");
    }
    if (!ensure_connected()) {
        return false;
    }
    setNonBlocking(false);

    try {
        serverbound_packets::Hello(create_account).send(get_socket());
        if (create_account) {
            serverbound_packets::SignIn(name, password).send(get_socket());
            auto response =
                read_specific_clientbound_packet<clientbound_packets::SignInResponse>(get_socket());
            if (!response || !response->success) {
                disconnect("account creation rejected by the server");
                return false;
            }
        } else {
            serverbound_packets::Login(name, password).send(get_socket());
            auto response =
                read_specific_clientbound_packet<clientbound_packets::LoginResponse>(get_socket());
            if (!response || !response->success) {
                disconnect("login rejected by the server");
                return false;
            }
        }

        setNonBlocking(true);
        {
            std::lock_guard<std::mutex> state_lock(state_mutex);
            authenticated = true;
            last_error.clear();
        }
        return true;
    } catch (const std::exception &error) {
        disconnect(error.what());
        return false;
    }
}

void Client::setNonBlocking(bool nonBlocking) {
    const int descriptor = get_socket();
    if (descriptor < 0 || set_nonblocking_fd(descriptor, nonBlocking)) {
        return;
    }
    set_error(std::string("failed to set socket mode: ") + std::strerror(errno));
}

bool Client::get_flag(void) {
    if (flag) {
        return true;
    }
    flag = true;
    return false;
}

void Client::switch_flag(void) {
    flag = false;
}

bool Client::connect_create(std::string name, std::string password) {
    return authenticate(name, password, true);
}

bool Client::connect_log(std::string name, std::string password) {
    return authenticate(name, password, false);
}

void Client::sendRequest(std::string friendName) {
    send_packet(serverbound_packets::SendRequest(std::move(friendName)));
}

void Client::gestion_friend(std::string friendName, bool accepted) {
    send_packet(serverbound_packets::HandleFriendRequest(std::move(friendName), accepted));
}

std::vector<std::string> Client::handle_my_requests() {
    std::vector<std::string> requests;
    std::lock_guard<std::mutex> io_lock(io_mutex);
    if (!ensure_connected()) {
        return requests;
    }
    try {
        setNonBlocking(false);
        serverbound_packets::GetMyRequests().send(get_socket());
        auto response =
            read_specific_clientbound_packet<clientbound_packets::GetMyRequests>(get_socket());
        setNonBlocking(true);
        if (response) {
            requests = response->friendRequests;
        }
    } catch (const std::exception &error) {
        disconnect(error.what());
    }
    return requests;
}

std::shared_ptr<Packet> Client::recepteur(void) {
    if (!is_connected()) {
        return nullptr;
    }
    const int descriptor = get_socket();
    try {
        // read_clientbound_packet uses the non-blocking descriptor and keeps
        // an incomplete packet in its per-socket buffer.
        auto packet = read_clientbound_packet(descriptor);
        if (auto room_info = std::dynamic_pointer_cast<clientbound_packets::RoomInfos>(packet)) {
            std::lock_guard<std::mutex> state_lock(state_mutex);
            if (room_info->reponse) {
                current_room = room_info->room_name;
                player_count = std::max(0, room_info->numb_player);
            } else {
                current_room.clear();
                player_count = 0;
                is_owner = false;
            }
        }
        return packet;
    } catch (const transport::WouldBlock &) {
        return nullptr;
    } catch (const std::exception &error) {
        disconnect(error.what());
        return nullptr;
    }
}

void Client::exit(void) {
    std::lock_guard<std::mutex> io_lock(io_mutex);
    const int descriptor = get_socket();
    if (descriptor >= 0 && is_connected()) {
        try {
            setNonBlocking(false);
            serverbound_packets::Hello(false).send(descriptor);
        } catch (const std::exception &error) {
            set_error(error.what());
        }
    }
    disconnect();
}

void Client::change_status(GameMode quoi) {
    send_packet(serverbound_packets::SetStatus(quoi));
}

void Client::send_message(std::string who_recv, std::string message) {
    send_packet(serverbound_packets::Message(std::move(who_recv), std::move(message)));
}

void Client::send_grid(std::vector<std::vector<int>> grid) {
    send_packet(serverbound_packets::GridData(std::move(grid)));
}

void Client::send_message_room(std::string room_name, std::string message) {
    send_packet(serverbound_packets::MessageRoom(std::move(room_name), std::move(message)));
}

void Client::create_room(int limit, std::string room_name, GameMode game_mode) {
    if (send_packet(serverbound_packets::CreateRoom(limit, room_name, game_mode))) {
        std::lock_guard<std::mutex> state_lock(state_mutex);
        current_room = std::move(room_name);
        player_count = 1;
        is_owner = true;
    }
}

void Client::join_room(std::string room_name) {
    if (send_packet(serverbound_packets::JoinRoom(room_name))) {
        std::lock_guard<std::mutex> state_lock(state_mutex);
        current_room = std::move(room_name);
        player_count = std::max(1, player_count + 1);
        is_owner = false;
    }
}

void Client::start_game(GameMode game_mode) {
    send_packet(serverbound_packets::SetReady(game_mode));
}

void Client::end_game(bool end) {
    send_packet(serverbound_packets::GameEnd(end));
}

void Client::send_input(int input) {
    send_packet(serverbound_packets::Input(input));
}

void Client::send_malus(Malus effect, int target) {
    send_packet(serverbound_packets::MalusPacket(effect, target));
}

int Client::get_player_number(void) {
    std::lock_guard<std::mutex> state_lock(state_mutex);
    return player_number;
}

void Client::set_player_number(int num) {
    std::lock_guard<std::mutex> state_lock(state_mutex);
    player_number = num;
}

void Client::send_score(int score) {
    send_packet(serverbound_packets::ScorePacket(score, 0));
}

int Client::request_score() {
    std::lock_guard<std::mutex> io_lock(io_mutex);
    if (!ensure_connected()) {
        return 0;
    }
    try {
        setNonBlocking(false);
        serverbound_packets::ScoreRequest().send(get_socket());
        auto response =
            read_specific_clientbound_packet<clientbound_packets::ScoreResponse>(get_socket());
        setNonBlocking(true);
        return response ? response->score : 0;
    } catch (const std::exception &error) {
        disconnect(error.what());
        return 0;
    }
}

std::vector<std::string> Client::friends_list() {
    std::vector<std::string> list;
    std::lock_guard<std::mutex> io_lock(io_mutex);
    if (!ensure_connected()) {
        return list;
    }
    try {
        setNonBlocking(false);
        serverbound_packets::FriendsListRequest().send(get_socket());
        auto response = read_specific_clientbound_packet<clientbound_packets::FriendsListResponse>(
            get_socket());
        setNonBlocking(true);
        if (response) {
            list = response->friendsList;
        }
    } catch (const std::exception &error) {
        disconnect(error.what());
    }
    return list;
}

std::vector<std::string> Client::top_scores_list() {
    std::vector<std::string> topScores;
    std::lock_guard<std::mutex> io_lock(io_mutex);
    if (!ensure_connected()) {
        return topScores;
    }
    try {
        setNonBlocking(false);
        serverbound_packets::ScoreListRequest().send(get_socket());
        auto response =
            read_specific_clientbound_packet<clientbound_packets::ScoreListResponse>(get_socket());
        setNonBlocking(true);
        if (response) {
            topScores = response->topScoreList;
        }
    } catch (const std::exception &error) {
        disconnect(error.what());
    }
    return topScores;
}

std::string Client::decouvrirServeurLAN(unsigned short discovery_port, int timeout_ms) {
    const int descriptor = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (descriptor < 0) {
        return {};
    }
    set_close_on_exec(descriptor);

    int broadcast = 1;
    ::setsockopt(descriptor, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    int reuse = 1;
    ::setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // Use an ephemeral local port.  The server owns discovery_port and sends
    // its response back to the requester's source port.
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(0);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    if (::bind(descriptor, reinterpret_cast<sockaddr *>(&local), sizeof(local)) < 0) {
        ::close(descriptor);
        return {};
    }

    sockaddr_in loopback{};
    loopback.sin_family = AF_INET;
    loopback.sin_port = htons(discovery_port);
    ::inet_pton(AF_INET, "127.0.0.1", &loopback.sin_addr);
    ::sendto(descriptor, kDiscoveryRequest, sizeof(kDiscoveryRequest) - 1, 0,
             reinterpret_cast<sockaddr *>(&loopback), sizeof(loopback));

    sockaddr_in broadcast_address{};
    broadcast_address.sin_family = AF_INET;
    broadcast_address.sin_port = htons(discovery_port);
    broadcast_address.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    ::sendto(descriptor, kDiscoveryRequest, sizeof(kDiscoveryRequest) - 1, 0,
             reinterpret_cast<sockaddr *>(&broadcast_address), sizeof(broadcast_address));

    const int bounded_timeout = std::max(1, timeout_ms);
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(bounded_timeout);
    std::array<char, 128> buffer{};
    while (std::chrono::steady_clock::now() < deadline) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        struct pollfd descriptor_poll{descriptor, POLLIN, 0};
        int ready;
        do {
            ready = ::poll(&descriptor_poll, 1,
                           static_cast<int>(std::max<std::int64_t>(1, remaining.count())));
        } while (ready < 0 && errno == EINTR);
        if (ready <= 0) {
            break;
        }

        sockaddr_in sender{};
        socklen_t sender_size = sizeof(sender);
        const ssize_t received = ::recvfrom(descriptor, buffer.data(), buffer.size(), 0,
                                            reinterpret_cast<sockaddr *>(&sender), &sender_size);
        if (received != static_cast<ssize_t>(sizeof(kDiscoveryResponse) - 1) ||
            std::memcmp(buffer.data(), kDiscoveryResponse, sizeof(kDiscoveryResponse) - 1) != 0) {
            continue;
        }

        char address[INET_ADDRSTRLEN]{};
        if (::inet_ntop(AF_INET, &sender.sin_addr, address, sizeof(address)) != nullptr) {
            ::close(descriptor);
            return address;
        }
    }

    ::close(descriptor);
    return {};
}

} // namespace tetris
