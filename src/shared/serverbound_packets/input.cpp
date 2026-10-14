#include <memory>
#include <ostream>

#include <read.hpp>
#include <serverbound_packets/input.hpp>
#include <write.hpp>

namespace tetris::serverbound_packets {
void Input::serialize(int socket) const {
    write(socket, ID);
    write(socket, input);
};

std::shared_ptr<Input> Input::recv(int socket) {
    int input;
    read(socket, input);
    return std::make_shared<Input>(input);
}
} // namespace tetris::serverbound_packets