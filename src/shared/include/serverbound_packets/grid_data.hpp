#pragma once

#include <memory>
#include <packet.hpp>
#include <vector>

namespace tetris::serverbound_packets {
class GridData : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::GridData;
    std::vector<std::vector<int>> grid;
    GridData(std::vector<std::vector<int>> grid) : grid(grid) {}
    static std::shared_ptr<GridData> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets
