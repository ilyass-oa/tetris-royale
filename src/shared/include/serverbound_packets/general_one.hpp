#pragma once

#include <memory>
#include <packet.hpp>

namespace tetris::serverbound_packets {
class GenO : public Packet {
  public:
    static constexpr ServerboundPacket ID = ServerboundPacket::GenO;
    int choice;
    bool my_friend;
    std::string who_recv;
    std::string message;
    GenO(int choice, bool my_friend, std::string who_recv, std::string message)
        : choice{choice}, my_friend(my_friend), who_recv{std::move(who_recv)},
          message{std::move(message)} {}
    static std::shared_ptr<GenO> recv(int socket);
    virtual void serialize(int socket) const override;
};
} // namespace tetris::serverbound_packets