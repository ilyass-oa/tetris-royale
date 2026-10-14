#pragma once

#include <memory>

#include <clientbound_packets/hello.hpp>
#include <serverbound_packets/hello.hpp>

namespace tetris {
class ServerPacketHandler {
  public:
    virtual void handle(std::shared_ptr<serverbound_packets::Hello> packet) = 0;
};

class ClientPacketHandler {
  public:
    virtual void handle(std::shared_ptr<clientbound_packets::Hello> packet) = 0;
};
} // namespace tetris
