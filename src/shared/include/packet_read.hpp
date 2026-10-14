#pragma once
#include <memory>
#include <packet.hpp>
#include <read.hpp>
#include <transport.hpp>
#include <type_traits>

namespace tetris {
std::shared_ptr<Packet> read_serverbound_packet(int socket, int timeout_ms = -1);
std::shared_ptr<Packet> read_clientbound_packet(int socket, int timeout_ms = -1);

// TODO: Try to unify this
template <class P> std::shared_ptr<P> read_specific_serverbound_packet(int socket) {
    static_assert(std::is_base_of<Packet, P>::value, "P must derive from Packet");
    auto packet = read_serverbound_packet(socket, transport::kDefaultReadTimeoutMs);
    if (!packet) {
        throw transport::WouldBlock("packet is incomplete");
    }
    auto expected = std::dynamic_pointer_cast<P>(packet);
    if (!expected) {
        throw transport::ProtocolError("received an unexpected serverbound packet");
    }
    return expected;
}

template <class P> std::shared_ptr<P> read_specific_clientbound_packet(int socket) {
    static_assert(std::is_base_of<Packet, P>::value, "P must derive from Packet");
    auto packet = read_clientbound_packet(socket, transport::kDefaultReadTimeoutMs);
    if (!packet) {
        throw transport::WouldBlock("packet is incomplete");
    }
    auto expected = std::dynamic_pointer_cast<P>(packet);
    if (!expected) {
        throw transport::ProtocolError("received an unexpected clientbound packet");
    }
    return expected;
}

} // namespace tetris
