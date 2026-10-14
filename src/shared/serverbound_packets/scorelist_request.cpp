#include <iostream>
#include <memory>
#include <read.hpp>
#include <serverbound_packets/scorelist_request.hpp>
#include <vector>
#include <write.hpp>

namespace tetris::serverbound_packets {

void ScoreListRequest::serialize(int socket) const {
    write(socket, ID);
}

std::shared_ptr<ScoreListRequest> ScoreListRequest::recv(int socket) {
    return std::make_shared<ScoreListRequest>();
}

} // namespace tetris::serverbound_packets
