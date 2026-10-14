#include <iostream>
#include <memory>

#include <read.hpp>
#include <serverbound_packets/grid_data.hpp>
#include <vector>
#include <write.hpp>

namespace tetris::serverbound_packets {
void GridData::serialize(int socket) const {
    write(socket, ID);
    write(socket, grid);
};
std::shared_ptr<GridData> GridData::recv(int socket) {
    std::vector<std::vector<int>> grid;
    read(socket, grid);
    return std::make_shared<GridData>(grid);
}
} // namespace tetris::serverbound_packets
