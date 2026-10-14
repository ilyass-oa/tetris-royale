#include <iostream>
#include <memory>
#include <read.hpp>
#include <serverbound_packets/friendslist_request.hpp>
#include <vector>
#include <write.hpp>

namespace tetris::serverbound_packets {

void FriendsListRequest::serialize(int socket) const {
    write(socket, ID);
}

std::shared_ptr<FriendsListRequest> FriendsListRequest::recv(int socket) {
    return std::make_shared<FriendsListRequest>();
}

} // namespace tetris::serverbound_packets