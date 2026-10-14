#include <memory>
#include <ostream>

#include <clientbound_packets/message_room.hpp>
#include <read.hpp>
#include <write.hpp>

namespace tetris::clientbound_packets {
void MessageRoom::serialize(int socket) const {
    write(socket, ID);
    write(socket, texte);
};

std::shared_ptr<MessageRoom> MessageRoom::recv(int socket) {
    std::string texte;
    read(socket, texte);

    return std::make_shared<MessageRoom>(texte);
}
} // namespace tetris::clientbound_packets
