#include <iostream>
#include <memory>

#include <clientbound_packets/general_response.hpp>
#include <read.hpp>
#include <write.hpp>

namespace tetris::clientbound_packets {
void GeneralResponse::serialize(int socket) const {
    write(socket, ClientboundPacket::GeneralResponse);
    write(socket, choice);
    write(socket, success);
    write(socket, message);
};
std::shared_ptr<GeneralResponse> GeneralResponse::recv(int socket) {
    int choice;
    bool success;
    std::string message;

    read(socket, choice);
    read(socket, success);
    read(socket, message);
    return std::make_shared<GeneralResponse>(choice, success, message);
}
} // namespace tetris::clientbound_packets
