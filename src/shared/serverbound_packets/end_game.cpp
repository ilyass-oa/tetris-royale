
#include <iostream>
#include <memory>

#include <read.hpp>
#include <serverbound_packets/end_game.hpp>
#include <vector>
#include <write.hpp>

namespace tetris::serverbound_packets {
void GameEnd::serialize(int socket) const {
    write(socket, ID);
    write(socket, end);
};
std::shared_ptr<GameEnd> GameEnd::recv(int socket) {
    bool end;
    read(socket, end);
    return std::make_shared<GameEnd>(end);
}
} // namespace tetris::serverbound_packets