#include <iostream>
#include <memory>
#include <read.hpp>
#include <serverbound_packets/get_my_requests.hpp>
#include <vector>
#include <write.hpp>

namespace tetris::serverbound_packets {

void GetMyRequests::serialize(int socket) const {
    write(socket, ID);
}

std::shared_ptr<GetMyRequests> GetMyRequests::recv(int socket) {
    return std::make_shared<GetMyRequests>();
}

} // namespace tetris::serverbound_packets
