#include "serveur.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <unistd.h>
namespace tetris::serveur {
namespace {
int listeningSocket(int type, const std::string &address, unsigned short port) {
    int fd = ::socket(AF_INET, type, 0);
    if (fd < 0)
        throw std::runtime_error("Cannot create server socket");
    try {
        int yes = 1;
        if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
            throw std::runtime_error("Cannot configure socket");
        sockaddr_in endpoint{};
        endpoint.sin_family = AF_INET;
        endpoint.sin_port = htons(port);
        if (::inet_pton(AF_INET, address.c_str(), &endpoint.sin_addr) != 1)
            throw std::runtime_error("Invalid bind IPv4 address");
        if (::bind(fd, reinterpret_cast<sockaddr *>(&endpoint), sizeof(endpoint)) < 0)
            throw std::runtime_error("Cannot bind port " + std::to_string(port) + ": " +
                                     std::strerror(errno));
        if (type == SOCK_STREAM && ::listen(fd, 64) < 0)
            throw std::runtime_error("Cannot listen");
        if (::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK) < 0)
            throw std::runtime_error("Cannot set nonblocking socket");
    } catch (...) {
        ::close(fd);
        throw;
    }
    return fd;
}
} // namespace
Serveur::Serveur(ServerConfig config)
    : config(std::move(config)), players(std::make_shared<PlayersManager>()),
      rooms(std::make_shared<RoomsManager>()),
      data(std::make_shared<DatabaseManager>(this->config.database_path)) {
    try {
        listener = listeningSocket(SOCK_STREAM, this->config.bind_address, this->config.port);
        if (this->config.discovery_port)
            discovery =
                listeningSocket(SOCK_DGRAM, this->config.bind_address, this->config.discovery_port);
    } catch (...) {
        if (listener >= 0)
            ::close(listener);
        throw;
    }
}
Serveur::~Serveur() {
    power_off();
}
void Serveur::answer_discovery() {
    char buffer[128];
    sockaddr_in peer{};
    socklen_t length = sizeof(peer);
    auto count = ::recvfrom(discovery, buffer, sizeof(buffer), 0,
                            reinterpret_cast<sockaddr *>(&peer), &length);
    if (count < 0)
        return;
    constexpr char request[] = "TETRIS_DISCOVER_LAN_V1";
    constexpr char response[] = "TETRIS_SERVER_LAN_V1";
    if (std::string_view(buffer, count) == request)
        ::sendto(discovery, response, sizeof(response) - 1, 0, reinterpret_cast<sockaddr *>(&peer),
                 length);
}
void Serveur::run(const std::atomic<bool> &stop) {
    std::cout << "Tetris server listening on " << config.bind_address << ':' << config.port
              << std::endl;
    while (!stop.load()) {
        for (auto it = sessions.begin(); it != sessions.end();) {
            if (it->player->is_finished())
                it = sessions.erase(it);
            else
                ++it;
        }
        pollfd fds[] = {{listener, POLLIN, 0}, {discovery, POLLIN, 0}};
        int result = ::poll(fds, discovery >= 0 ? 2 : 1, 100);
        if (result < 0) {
            if (errno == EINTR)
                continue;
            throw std::runtime_error("Server poll failed");
        }
        if (discovery >= 0 && fds[1].revents & POLLIN)
            answer_discovery();
        if (!(fds[0].revents & POLLIN))
            continue;
        sockaddr_in address{};
        socklen_t length = sizeof(address);
        int fd = ::accept(listener, reinterpret_cast<sockaddr *>(&address), &length);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                continue;
            throw std::runtime_error("Accept failed");
        }
        if (sessions.size() >= 64) {
            ::close(fd);
            continue;
        }
        auto player = std::make_shared<Player>(fd, address, players, rooms, data);
        try {
            sessions.push_back({player, std::jthread([player] { player->player_thread(); })});
        } catch (...) {
            player->exit();
            throw;
        }
    }
    power_off();
}
void Serveur::power_off() {
    if (listener >= 0) {
        ::close(listener);
        listener = -1;
    }
    if (discovery >= 0) {
        ::close(discovery);
        discovery = -1;
    }
    for (auto &session : sessions)
        session.player->exit();
    rooms->shutdown();
    sessions.clear();
}
} // namespace tetris::serveur
