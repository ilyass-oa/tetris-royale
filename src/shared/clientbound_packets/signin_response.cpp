#include <iostream>
#include <memory>

#include <clientbound_packets/signin_response.hpp>
#include <read.hpp>
#include <write.hpp>

namespace tetris::clientbound_packets {
void SignInResponse::serialize(int socket) const {
    write(socket, ClientboundPacket::SignInResponse);
    write(socket, success);
};
std::shared_ptr<SignInResponse> SignInResponse::recv(int socket) {
    bool success;
    read(socket, success);
    return std::make_shared<SignInResponse>(success);
}
} // namespace tetris::clientbound_packets
