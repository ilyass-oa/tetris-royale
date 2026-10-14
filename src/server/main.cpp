#include "gestion_serveur/serveur.hpp"
#include <atomic>
#include <csignal>
#include <iostream>
#include <limits>
namespace {
static_assert(std::atomic<bool>::is_always_lock_free);
std::atomic<bool> stopped{false};
void handleSignal(int) {
    stopped.store(true, std::memory_order_relaxed);
}
unsigned short port(const std::string &value) {
    std::size_t used = 0;
    unsigned long n = std::stoul(value, &used);
    if (used != value.size() || n == 0 || n > 65535)
        throw std::runtime_error("Port must be 1..65535");
    return static_cast<unsigned short>(n);
}
} // namespace
int main(int argc, char **argv) {
    try {
        tetris::serveur::ServerConfig config;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--help") {
                std::cout << "server [--bind IPv4] [--port N] [--discovery-port N] "
                             "[--no-discovery] [--data PATH]\n";
                return 0;
            }
            if (arg == "--no-discovery") {
                config.discovery_port = 0;
                continue;
            }
            if (i + 1 >= argc)
                throw std::runtime_error("Missing argument value");
            std::string value = argv[++i];
            if (arg == "--bind")
                config.bind_address = value;
            else if (arg == "--port")
                config.port = port(value);
            else if (arg == "--discovery-port")
                config.discovery_port = port(value);
            else if (arg == "--data")
                config.database_path = value;
            else
                throw std::runtime_error("Unknown argument: " + arg);
        }
        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);
        tetris::serveur::Serveur server(config);
        server.run(stopped);
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Server error: " << e.what() << '\n';
        return 1;
    }
}
