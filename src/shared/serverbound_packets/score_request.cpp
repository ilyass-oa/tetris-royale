#include <iostream>
#include <memory>
#include <serverbound_packets/score_request.hpp>

#include <read.hpp>
#include <write.hpp>

namespace tetris::serverbound_packets {
std::shared_ptr<ScoreRequest> ScoreRequest::recv(int socket) {
    return std::make_shared<ScoreRequest>();
}

void ScoreRequest::serialize(int socket) const {
    write(socket, ID);
}
} // namespace tetris::serverbound_packets
