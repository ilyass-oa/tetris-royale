#include <packet_read.hpp>
#include <packets_include.hpp>
#include <transport.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <memory>
#include <mutex>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tetris {
namespace {

struct SocketState {
    std::mutex mutex;
    std::vector<unsigned char> input;
    std::chrono::steady_clock::time_point partial_since{};
};

std::mutex states_mutex;
std::unordered_map<int, std::shared_ptr<SocketState>> states;

std::shared_ptr<SocketState> state_for(int socket) {
    std::lock_guard<std::mutex> lock(states_mutex);
    auto [it, inserted] = states.emplace(socket, nullptr);
    if (inserted || !it->second) {
        it->second = std::make_shared<SocketState>();
    }
    return it->second;
}

bool socket_is_nonblocking(int socket) {
    const int flags = ::fcntl(socket, F_GETFL, 0);
    return flags >= 0 && (flags & O_NONBLOCK) != 0;
}

struct ParseContext {
    int socket;
    const std::vector<unsigned char> *input;
    std::size_t offset = 0;
};

thread_local ParseContext *active_context = nullptr;

bool receive_more(int socket, std::vector<unsigned char> &input, bool nonblocking,
                  const std::chrono::steady_clock::time_point *deadline) {
    if (input.size() >= transport::kMaxBufferedBytes) {
        throw transport::ProtocolError("packet exceeds the protocol buffer limit");
    }

    std::array<unsigned char, 16 * 1024> chunk{};
    for (;;) {
        int flags = 0;
        if (nonblocking || deadline != nullptr) {
            flags |= MSG_DONTWAIT;
        }

        const ssize_t received = ::recv(socket, chunk.data(), chunk.size(), flags);
        if (received > 0) {
            const auto count = static_cast<std::size_t>(received);
            if (count > transport::kMaxBufferedBytes - input.size()) {
                throw transport::ProtocolError("packet exceeds the protocol buffer limit");
            }
            input.insert(input.end(), chunk.begin(), chunk.begin() + count);
            return true;
        }
        if (received == 0) {
            throw transport::ConnectionClosed("peer closed the socket");
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            if (nonblocking && deadline == nullptr) {
                return false;
            }

            struct pollfd descriptor{socket, POLLIN, 0};
            int ready;
            int timeout = -1;
            if (deadline != nullptr) {
                const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                    *deadline - std::chrono::steady_clock::now());
                if (remaining.count() <= 0) {
                    throw transport::WouldBlock("read deadline exceeded");
                }
                timeout = static_cast<int>(
                    std::min<std::int64_t>(remaining.count(), std::numeric_limits<int>::max()));
            }
            do {
                ready = ::poll(&descriptor, 1, timeout);
            } while (ready < 0 && errno == EINTR);
            if (ready == 0) {
                throw transport::WouldBlock("read deadline exceeded");
            }
            if (ready < 0) {
                throw transport::Error(std::string("poll failed while reading: ") +
                                       std::strerror(errno));
            }
            // POLLIN can accompany POLLHUP: drain final bytes before recv() reports EOF.
            if ((descriptor.revents & POLLNVAL) ||
                ((descriptor.revents & POLLERR) && !(descriptor.revents & POLLIN))) {
                throw transport::ConnectionClosed("socket became unavailable while reading");
            }
            continue;
        }
        throw transport::Error(std::string("recv failed: ") + std::strerror(errno));
    }
}

template <typename Parser>
std::shared_ptr<Packet> read_buffered_packet(int socket, Parser &&parser, int timeout_ms) {
    if (socket < 0) {
        throw transport::Error("invalid socket descriptor");
    }

    const auto state = state_for(socket);
    std::unique_lock<std::mutex> lock(state->mutex);
    // A zero timeout is an explicit prompt poll.  Treat it as non-blocking
    // even when the caller supplied a blocking descriptor (the GUI uses this
    // mode while polling one active reader).  Positive timeouts remain
    // bounded reads, while -1 retains the server's indefinite idle wait.
    const bool prompt_only = timeout_ms == 0;
    const bool nonblocking = socket_is_nonblocking(socket) || prompt_only;
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(std::max(0, timeout_ms));
    const auto *deadline_ptr = timeout_ms > 0 ? &deadline : nullptr;

    for (;;) {
        ParseContext context{socket, &state->input};
        active_context = &context;
        try {
            auto packet = parser(socket);
            active_context = nullptr;
            if (context.offset == 0) {
                throw transport::ProtocolError("packet parser consumed no bytes");
            }
            state->input.erase(state->input.begin(),
                               state->input.begin() + static_cast<std::ptrdiff_t>(context.offset));
            state->partial_since = {};
            return packet;
        } catch (const transport::WouldBlock &) {
            active_context = nullptr;
        } catch (...) {
            active_context = nullptr;
            throw;
        }

        // A non-blocking caller gets a prompt nullptr until the complete
        // packet arrives.  The input vector remains untouched, including any
        // packet prefix already parsed by the failed attempt.
        auto partial_deadline = deadline;
        auto effective_deadline = deadline_ptr;
        if (!state->input.empty()) {
            auto now = std::chrono::steady_clock::now();
            if (state->partial_since == std::chrono::steady_clock::time_point{})
                state->partial_since = now;
            partial_deadline =
                state->partial_since + std::chrono::milliseconds(transport::kDefaultReadTimeoutMs);
            if (now >= partial_deadline)
                throw transport::ProtocolError("partial packet deadline exceeded");
            if (!effective_deadline || partial_deadline < *effective_deadline)
                effective_deadline = &partial_deadline;
        }
        // A prompt UI poll must never wait for the partial-packet deadline.
        if (nonblocking && timeout_ms <= 0)
            effective_deadline = nullptr;
        if (!receive_more(socket, state->input, nonblocking, effective_deadline)) {
            return nullptr;
        }
    }
}

std::shared_ptr<Packet> parse_serverbound_packet(int socket) {
    ServerboundPacket id;
    read(socket, id);
    switch (id) {
    case ServerboundPacket::GenO:
        return std::static_pointer_cast<Packet>(serverbound_packets::GenO::recv(socket));
    case ServerboundPacket::Hello:
        return std::static_pointer_cast<Packet>(serverbound_packets::Hello::recv(socket));
    case ServerboundPacket::Login:
        return std::static_pointer_cast<Packet>(serverbound_packets::Login::recv(socket));
    case ServerboundPacket::SignIn:
        return std::static_pointer_cast<Packet>(serverbound_packets::SignIn::recv(socket));
    case ServerboundPacket::Message:
        return std::static_pointer_cast<Packet>(serverbound_packets::Message::recv(socket));
    case ServerboundPacket::MessageRoom:
        return std::static_pointer_cast<Packet>(serverbound_packets::MessageRoom::recv(socket));
    case ServerboundPacket::GridData:
        return std::static_pointer_cast<Packet>(serverbound_packets::GridData::recv(socket));
    case ServerboundPacket::MalusPacket:
        return std::static_pointer_cast<Packet>(serverbound_packets::MalusPacket::recv(socket));
    case ServerboundPacket::CreateRoom:
        return std::static_pointer_cast<Packet>(serverbound_packets::CreateRoom::recv(socket));
    case ServerboundPacket::JoinRoom:
        return std::static_pointer_cast<Packet>(serverbound_packets::JoinRoom::recv(socket));
    case ServerboundPacket::DelRoom:
        return std::static_pointer_cast<Packet>(serverbound_packets::DelRoom::recv(socket));
    case ServerboundPacket::RoomInfos:
        return std::static_pointer_cast<Packet>(serverbound_packets::RoomInfos::recv(socket));
    case ServerboundPacket::SetReady:
        return std::static_pointer_cast<Packet>(serverbound_packets::SetReady::recv(socket));
    case ServerboundPacket::SetStatus:
        return std::static_pointer_cast<Packet>(serverbound_packets::SetStatus::recv(socket));
    case ServerboundPacket::GameEnd:
        return std::static_pointer_cast<Packet>(serverbound_packets::GameEnd::recv(socket));
    case ServerboundPacket::ScorePacket:
        return std::static_pointer_cast<Packet>(serverbound_packets::ScorePacket::recv(socket));
    case ServerboundPacket::ScoreRequest:
        return std::static_pointer_cast<Packet>(serverbound_packets::ScoreRequest::recv(socket));
    case ServerboundPacket::SendRequest:
        return std::static_pointer_cast<Packet>(serverbound_packets::SendRequest::recv(socket));
    case ServerboundPacket::GetMyRequests:
        return std::static_pointer_cast<Packet>(serverbound_packets::GetMyRequests::recv(socket));
    case ServerboundPacket::HandleFriendRequest:
        return std::static_pointer_cast<Packet>(
            serverbound_packets::HandleFriendRequest::recv(socket));
    case ServerboundPacket::FriendsListRequest:
        return std::static_pointer_cast<Packet>(
            serverbound_packets::FriendsListRequest::recv(socket));
    case ServerboundPacket::ScoreListRequest:
        return std::static_pointer_cast<Packet>(
            serverbound_packets::ScoreListRequest::recv(socket));
    case ServerboundPacket::Input:
        return std::static_pointer_cast<Packet>(serverbound_packets::Input::recv(socket));
    }
    throw transport::ProtocolError("unhandled serverbound packet type");
}

std::shared_ptr<Packet> parse_clientbound_packet(int socket) {
    ClientboundPacket id;
    read(socket, id);
    switch (id) {
    case ClientboundPacket::GeneralResponse:
        return std::static_pointer_cast<Packet>(clientbound_packets::GeneralResponse::recv(socket));
    case ClientboundPacket::Hello:
        return std::static_pointer_cast<Packet>(clientbound_packets::Hello::recv(socket));
    case ClientboundPacket::LoginResponse:
        return std::static_pointer_cast<Packet>(clientbound_packets::LoginResponse::recv(socket));
    case ClientboundPacket::SignInResponse:
        return std::static_pointer_cast<Packet>(clientbound_packets::SignInResponse::recv(socket));
    case ClientboundPacket::Message:
        return std::static_pointer_cast<Packet>(clientbound_packets::Message::recv(socket));
    case ClientboundPacket::GridData:
        return std::static_pointer_cast<Packet>(clientbound_packets::GridData::recv(socket));
    case ClientboundPacket::Malus:
        return std::static_pointer_cast<Packet>(clientbound_packets::Malus::recv(socket));
    case ClientboundPacket::RoomInfos:
        return std::static_pointer_cast<Packet>(clientbound_packets::RoomInfos::recv(socket));
    case ClientboundPacket::GameStart:
        return std::static_pointer_cast<Packet>(clientbound_packets::GameStart::recv(socket));
    case ClientboundPacket::GameEnd:
        return std::static_pointer_cast<Packet>(clientbound_packets::GameEnd::recv(socket));
    case ClientboundPacket::ScoreResponse:
        return std::static_pointer_cast<Packet>(clientbound_packets::ScoreResponse::recv(socket));
    case ClientboundPacket::GetMyRequests:
        return std::static_pointer_cast<Packet>(clientbound_packets::GetMyRequests::recv(socket));
    case ClientboundPacket::FriendsListResponse:
        return std::static_pointer_cast<Packet>(
            clientbound_packets::FriendsListResponse::recv(socket));
    case ClientboundPacket::ScoreListResponse:
        return std::static_pointer_cast<Packet>(
            clientbound_packets::ScoreListResponse::recv(socket));
    case ClientboundPacket::NextTile:
        return std::static_pointer_cast<Packet>(clientbound_packets::NextTile::recv(socket));
    case ClientboundPacket::ScorePacket:
        return std::static_pointer_cast<Packet>(clientbound_packets::ScorePacket::recv(socket));
    case ClientboundPacket::AddFriendResponse:
    case ClientboundPacket::RemoveFriendResponse:
        throw transport::ProtocolError("unsupported clientbound packet type");
    }
    throw transport::ProtocolError("unhandled clientbound packet type");
}

} // namespace

namespace transport {

void read_exact(int socket, void *buffer, std::size_t size) {
    if (size == 0) {
        return;
    }
    if (buffer == nullptr) {
        throw Error("cannot read into a null buffer");
    }

    if (active_context != nullptr && active_context->socket == socket) {
        const auto available = active_context->input->size() - active_context->offset;
        if (size > available) {
            throw WouldBlock("packet is incomplete");
        }
        std::memcpy(buffer, active_context->input->data() + active_context->offset, size);
        active_context->offset += size;
        return;
    }

    auto *bytes = static_cast<unsigned char *>(buffer);
    std::size_t received_total = 0;
    while (received_total < size) {
        const ssize_t received = ::recv(socket, bytes + received_total, size - received_total, 0);
        if (received > 0) {
            received_total += static_cast<std::size_t>(received);
            continue;
        }
        if (received == 0) {
            throw ConnectionClosed("peer closed the socket");
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            throw WouldBlock("socket has no complete value yet");
        }
        throw Error(std::string("recv failed: ") + std::strerror(errno));
    }
}

void forget_socket(int socket) noexcept {
    std::lock_guard<std::mutex> lock(states_mutex);
    states.erase(socket);
}

} // namespace transport

std::shared_ptr<Packet> read_serverbound_packet(int socket, int timeout_ms) {
    return read_buffered_packet(socket, parse_serverbound_packet, timeout_ms);
}

std::shared_ptr<Packet> read_clientbound_packet(int socket, int timeout_ms) {
    return read_buffered_packet(socket, parse_clientbound_packet, timeout_ms);
}

} // namespace tetris
