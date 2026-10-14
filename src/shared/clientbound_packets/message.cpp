#include <iostream>
#include <memory>

#include <clientbound_packets/message.hpp>
#include <read.hpp>
#include <write.hpp>

namespace tetris::clientbound_packets {
void Message::serialize(int socket) const {
    write(socket, ID);
    write(socket, texte);
};
std::shared_ptr<Message> Message::recv(int socket) {
    std::string texte;
    read(socket, texte);
    return std::make_shared<Message>(texte);
}
} // namespace tetris::clientbound_packets
