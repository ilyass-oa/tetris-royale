#include <memory>
#include <ostream>

#include <read.hpp>
#include <serverbound_packets/send_request.hpp>
#include <write.hpp>

namespace tetris::serverbound_packets {
std::shared_ptr<SendRequest> SendRequest::recv(int socket) {
    std::string friendName;
    read(socket, friendName);
    return std::make_shared<SendRequest>(friendName);
}

void SendRequest::serialize(int socket) const {
    write(socket, ID);
    write(socket, friendName);
}
} // namespace tetris::serverbound_packets
