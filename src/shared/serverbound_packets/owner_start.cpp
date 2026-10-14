#include <memory>
#include <ostream>

#include <read.hpp>
#include <serverbound_packets/owner_start.hpp>
#include <write.hpp>

namespace tetris::serverbound_packets {
void SetReady::serialize(int socket) const {
    write(socket, ID);
    write(socket, game_mode);
};

std::shared_ptr<SetReady> SetReady::recv(int socket) {
    GameMode game_mode;
    read(socket, game_mode);
    if (game_mode < GameMode::Menu || game_mode > GameMode::BR) {
        throw transport::ProtocolError("invalid game mode");
    }
    return std::make_shared<SetReady>(game_mode);
}
} // namespace tetris::serverbound_packets
