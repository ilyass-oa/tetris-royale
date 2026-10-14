#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::clientbound_packets {
class Hello : public Packet {
  private:
    std::string name;

  public:
    static constexpr ClientboundPacket ID = ClientboundPacket::Hello;
    Hello(std::string name) : name{name} {}
    static std::shared_ptr<Hello> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::clientbound_packets