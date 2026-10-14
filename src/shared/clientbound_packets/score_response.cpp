#include <clientbound_packets/score_response.hpp>
#include <memory>
#include <read.hpp>
#include <write.hpp>

namespace tetris::clientbound_packets {

std::shared_ptr<ScoreResponse> ScoreResponse::recv(int socket) {
    int score;
    read(socket, score);
    return std::make_shared<ScoreResponse>(score);
};

void ScoreResponse::serialize(int socket) const {
    write(socket, ID);
    write(socket, score);
}

} // namespace tetris::clientbound_packets
