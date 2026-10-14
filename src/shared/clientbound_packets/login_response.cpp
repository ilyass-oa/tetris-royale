#include <iostream>
#include <memory>

#include <clientbound_packets/login_response.hpp>
#include <read.hpp>
#include <write.hpp>

namespace tetris::clientbound_packets {
void LoginResponse::serialize(int socket) const {
    write(socket, ClientboundPacket::LoginResponse);
    write(socket, success);
};
std::shared_ptr<LoginResponse> LoginResponse::recv(int socket) {
    bool success;
    read(socket, success);
    return std::make_shared<LoginResponse>(success);
}
} // namespace tetris::clientbound_packets
