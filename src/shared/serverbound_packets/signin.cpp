#include <memory>
#include <ostream>

#include <read.hpp>
#include <serverbound_packets/signin.hpp>
#include <write.hpp>

namespace tetris::serverbound_packets {
void SignIn::serialize(int socket) const {
    write(socket, ID);
    write(socket, username);
    write(socket, password);
};

std::shared_ptr<SignIn> SignIn::recv(int socket) {
    std::string username;
    std::string password;
    read(socket, username);
    read(socket, password);
    return std::make_shared<SignIn>(username, password);
}
} // namespace tetris::serverbound_packets
