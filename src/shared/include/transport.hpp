#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace tetris::transport {

namespace detail {

template <typename T, bool IsEnum = std::is_enum_v<T>> struct wire_logical {
    using type = T;
};

template <typename T> struct wire_logical<T, true> {
    using type = std::underlying_type_t<T>;
};

template <typename T> using wire_logical_t = typename wire_logical<T>::type;

template <typename T, bool IsBool = std::is_same_v<T, bool>> struct wire_unsigned {
    using type = std::make_unsigned_t<T>;
};

template <typename T> struct wire_unsigned<T, true> {
    using type = std::uint8_t;
};

template <typename T> using wire_unsigned_t = typename wire_unsigned<T>::type;

template <typename T>
inline constexpr std::size_t wire_size_v =
    std::is_same_v<std::remove_cv_t<T>, std::size_t> ? sizeof(std::uint64_t)
                                                     : sizeof(wire_logical_t<std::remove_cv_t<T>>);

} // namespace detail

// These limits apply to every length-prefixed value decoded by the shared
// protocol.  They keep a peer from turning a malformed length into an
// unbounded allocation while still leaving enough room for normal game data.
inline constexpr std::size_t kMaxStringBytes = 64 * 1024;
inline constexpr std::size_t kMaxVectorElements = 4096;
inline constexpr std::size_t kMaxBufferedBytes = 1024 * 1024;
inline constexpr int kDefaultWriteTimeoutMs = 200;
inline constexpr int kDefaultReadTimeoutMs = 5000;

class Error : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

class WouldBlock final : public Error {
  public:
    using Error::Error;
};

class ConnectionClosed final : public Error {
  public:
    using Error::Error;
};

class ProtocolError final : public Error {
  public:
    using Error::Error;
};

// Read exactly `size` bytes.  When packet_read.cpp has an active buffered
// parser, this consumes bytes from that parser without touching the socket.
// Otherwise it reads from the socket and handles short reads and EINTR.
void read_exact(int socket, void *buffer, std::size_t size);

// Send exactly `size` bytes.  The implementation handles short writes,
// EINTR, EAGAIN/EWOULDBLOCK and suppresses SIGPIPE on platforms that support
// MSG_NOSIGNAL.
void send_all(int socket, const void *buffer, std::size_t size,
              int timeout_ms = kDefaultWriteTimeoutMs);

// Forget buffered input associated with a descriptor before it is closed or
// reused.  This is safe to call for a descriptor that has no buffered data.
void forget_socket(int socket) noexcept;

} // namespace tetris::transport
