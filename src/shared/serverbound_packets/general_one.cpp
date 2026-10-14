#include <memory>
#include <ostream>

#include <read.hpp>
#include <serverbound_packets/general_one.hpp>
#include <write.hpp>

namespace tetris::serverbound_packets {
void GenO::serialize(int socket) const {
    write(socket, ID);
    write(socket, choice);
    write(socket, my_friend);
    write(socket, who_recv);
    write(socket, message);
};

std::shared_ptr<GenO> GenO::recv(int socket) {
    int choice;
    bool my_friend;
    std::string who_recv;
    std::string message;

    read(socket, choice);
    read(socket, my_friend);
    read(socket, who_recv);
    read(socket, message);
    return std::make_shared<GenO>(choice, my_friend, who_recv, message);
}
} // namespace tetris::serverbound_packets
