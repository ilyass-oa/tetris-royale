#include <iostream>
#include <memory>

#include <clientbound_packets/hello.hpp>
#include <read.hpp>
#include <write.hpp>

namespace tetris::clientbound_packets {
void Hello::serialize(int socket) const {
    write(socket, ClientboundPacket::Hello);
    write(socket, name);
};
std::shared_ptr<Hello> Hello::recv(int socket) {
    std::string name;
    read(socket, name);
    return std::make_shared<Hello>(name);
}
} // namespace tetris::clientbound_packets
