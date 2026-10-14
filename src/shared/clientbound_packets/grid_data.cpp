#include <iostream>
#include <memory>

#include <clientbound_packets/grid_data.hpp>
#include <read.hpp>
#include <vector>
#include <write.hpp>

namespace tetris::clientbound_packets {
void GridData::serialize(int socket) const {
    write(socket, ClientboundPacket::GridData);
    write(socket, player);
    write(socket, grid);
};
std::shared_ptr<GridData> GridData::recv(int socket) {
    int player;
    std::vector<std::vector<int>> grid;
    read(socket, player);
    read(socket, grid);
    return std::make_shared<GridData>(player, grid);
}
} // namespace tetris::clientbound_packets
