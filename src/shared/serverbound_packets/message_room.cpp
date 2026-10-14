#include <memory>
#include <ostream>

#include <read.hpp>
#include <serverbound_packets/message_room.hpp>
#include <write.hpp>

namespace tetris::serverbound_packets {
void MessageRoom::serialize(int socket) const {
    write(socket, ID);
    write(socket, room);
    write(socket, texte);
};

std::shared_ptr<MessageRoom> MessageRoom::recv(int socket) {
    std::string room;
    std::string texte;

    read(socket, room);
    read(socket, texte);

    return std::make_shared<MessageRoom>(room, texte);
}
} // namespace tetris::serverbound_packets
