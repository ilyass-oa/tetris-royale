#include <iostream>
#include <memory>

#include <clientbound_packets/end_game.hpp>
#include <read.hpp>
#include <write.hpp>

namespace tetris::clientbound_packets {
void GameEnd::serialize(int socket) const {
    write(socket, ID);
    write(socket, end);
};
std::shared_ptr<GameEnd> GameEnd::recv(int socket) {
    bool end;
    read(socket, end);

    return std::make_shared<GameEnd>(end);
}
} // namespace tetris::clientbound_packets
