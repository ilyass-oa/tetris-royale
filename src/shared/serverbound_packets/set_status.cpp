#include <memory>
#include <ostream>

#include <read.hpp>
#include <serverbound_packets/set_status.hpp>
#include <write.hpp>

namespace tetris::serverbound_packets {
void SetStatus::serialize(int socket) const {
    write(socket, ID);
    write(socket, status);
};

std::shared_ptr<SetStatus> SetStatus::recv(int socket) {
    GameMode status;
    read(socket, status);
    if (status < GameMode::Menu || status > GameMode::BR) {
        throw transport::ProtocolError("invalid game mode");
    }
    return std::make_shared<SetStatus>(status);
}
} // namespace tetris::serverbound_packets
