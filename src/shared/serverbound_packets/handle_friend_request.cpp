#include <iostream>
#include <read.hpp>
#include <serverbound_packets/handle_friend_request.hpp>
#include <write.hpp>

namespace tetris::serverbound_packets {
std::shared_ptr<HandleFriendRequest> HandleFriendRequest::recv(int socket) {
    std::string friendName;
    bool accepted;
    read(socket, friendName);
    read(socket, accepted);
    return std::make_shared<HandleFriendRequest>(friendName, accepted);
}

void HandleFriendRequest::serialize(int socket) const {
    write(socket, ID);
    write(socket, friendName);
    write(socket, accepted);
}
} // namespace tetris::serverbound_packets
