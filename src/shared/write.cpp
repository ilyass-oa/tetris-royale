#include <packet.hpp>
#include <write.hpp>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <poll.h>
#include <sys/socket.h>

namespace tetris {
namespace {
struct EncodedPacket {
    int socket;
    std::vector<unsigned char> bytes;
};
thread_local EncodedPacket *encoding = nullptr;
} // namespace
void ensure_write(int socket, const void *data, size_t length) {
    if (encoding && encoding->socket == socket) {
        if (length > transport::kMaxBufferedBytes - encoding->bytes.size())
            throw transport::ProtocolError("packet exceeds output buffer limit");
        const auto *first = static_cast<const unsigned char *>(data);
        if (length)
            encoding->bytes.insert(encoding->bytes.end(), first, first + length);
    } else
        transport::send_all(socket, data, length);
}
void Packet::send(int socket) const {
    if (encoding)
        throw transport::Error("nested packet serialization");
    EncodedPacket packet{socket, {}};
    packet.bytes.reserve(2048);
    encoding = &packet;
    try {
        serialize(socket);
    } catch (...) {
        encoding = nullptr;
        throw;
    }
    encoding = nullptr;
    transport::send_all(socket, packet.bytes.data(), packet.bytes.size());
}

namespace transport {
void send_all(int socket, const void *buffer, std::size_t size, int timeout_ms) {
    if (size == 0) {
        return;
    }
    if (buffer == nullptr) {
        throw Error("cannot send a null buffer");
    }

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeout_ms < 0 ? 0 : timeout_ms);
    const auto *bytes = static_cast<const unsigned char *>(buffer);
    std::size_t written = 0;

    while (written < size) {
        int flags = MSG_DONTWAIT;
#ifdef MSG_NOSIGNAL
        flags |= MSG_NOSIGNAL;
#endif
        const ssize_t sent = ::send(socket, bytes + written, size - written, flags);
        if (sent > 0) {
            written += static_cast<std::size_t>(sent);
            continue;
        }
        if (sent == 0) {
            throw ConnectionClosed("socket closed while sending");
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            throw Error(std::string("send failed: ") + std::strerror(errno));
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            throw WouldBlock("send deadline exceeded");
        }
        const auto remaining =
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        struct pollfd descriptor{socket, POLLOUT, 0};
        const int wait_ms = static_cast<int>(std::max<std::int64_t>(1, remaining.count()));
        int ready;
        do {
            ready = ::poll(&descriptor, 1, wait_ms);
        } while (ready < 0 && errno == EINTR);
        if (ready == 0) {
            throw WouldBlock("send deadline exceeded");
        }
        if (ready < 0) {
            throw Error(std::string("poll failed while sending: ") + std::strerror(errno));
        }
        if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            throw ConnectionClosed("socket became unavailable while sending");
        }
    }
}
} // namespace transport
} // namespace tetris
