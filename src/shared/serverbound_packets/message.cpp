#include <memory>
#include <ostream>

#include <read.hpp>
#include <serverbound_packets/message.hpp>
#include <write.hpp>

namespace tetris::serverbound_packets {
void Message::serialize(int socket) const {
    write(socket, ID);
    write(socket, who);
    write(socket, texte);
};

std::shared_ptr<Message> Message::recv(int socket) {
    std::string who;
    std::string texte;

    read(socket, who);
    read(socket, texte);

    return std::make_shared<Message>(who, texte);
}
} // namespace tetris::serverbound_packets
