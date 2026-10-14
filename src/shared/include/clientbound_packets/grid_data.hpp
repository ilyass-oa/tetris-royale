#pragma once

#include <memory>
#include <packet.hpp>
#include <utility>
#include <vector>

namespace tetris::clientbound_packets {
class GridData : public Packet {
  public:
    static constexpr ClientboundPacket ID = ClientboundPacket::GridData;
    std::vector<std::vector<int>> grid;
    int player;
    GridData(int player, std::vector<std::vector<int>> grid)
        : grid(std::move(grid)), player(player) {}
    static std::shared_ptr<GridData> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::clientbound_packets
