#include <memory>
#include <ostream>

#include <read.hpp>
#include <serverbound_packets/hello.hpp>
#include <write.hpp>

namespace tetris::serverbound_packets {
void Hello::serialize(int socket) const {
    write(socket, ID);
    write(socket, create_account);
};

std::shared_ptr<Hello> Hello::recv(int socket) {
    bool create_account;
    read(socket, create_account);
    return std::make_shared<Hello>(create_account);
}
} // namespace tetris::serverbound_packets
