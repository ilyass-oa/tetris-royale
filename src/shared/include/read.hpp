#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <concept_simple.hpp>
#include <transport.hpp>

namespace tetris {

namespace detail {

template <typename T>
T decode_simple(const std::array<unsigned char, sizeof(std::uint64_t)> &bytes) {
    using value_type = std::remove_cv_t<T>;
    using logical_type = transport::detail::wire_logical_t<value_type>;
    using unsigned_type = transport::detail::wire_unsigned_t<logical_type>;

    constexpr std::size_t width = transport::detail::wire_size_v<value_type>;
    std::uint64_t raw = 0;
    for (std::size_t i = 0; i < width; ++i) {
        raw = (raw << 8u) | bytes[i];
    }

    if constexpr (std::is_same_v<value_type, std::size_t>) {
        if (raw > std::numeric_limits<std::size_t>::max()) {
            throw transport::ProtocolError("size does not fit in size_t");
        }
        return static_cast<std::size_t>(raw);
    } else if constexpr (std::is_same_v<logical_type, bool>) {
        if (raw > 1u) {
            throw transport::ProtocolError("invalid boolean value");
        }
        return static_cast<T>(raw != 0u);
    } else {
        unsigned_type bits = static_cast<unsigned_type>(raw);
        logical_type logical{};
        static_assert(sizeof(bits) <= sizeof(logical));
        std::memcpy(&logical, &bits, sizeof(bits));
        return static_cast<T>(logical);
    }
}

} // namespace detail

template <typename T>
    requires simple<T>
inline void read(int socket, T &out) {
    std::array<unsigned char, sizeof(std::uint64_t)> bytes{};
    transport::read_exact(socket, bytes.data(), transport::detail::wire_size_v<T>);
    out = detail::decode_simple<T>(bytes);
}

template <typename T> inline void read(int socket, std::vector<T> &out) {
    out.clear();
    std::size_t size;
    read(socket, size);
    if (size > transport::kMaxVectorElements) {
        throw transport::ProtocolError("vector exceeds the protocol limit");
    }
    out.reserve(size);
    for (std::size_t i = 0; i < size; i++) {
        T next;
        read(socket, next);
        out.push_back(std::move(next));
    }
}

inline void read(int socket, std::string &out) {
    out.clear();
    std::size_t size;
    read(socket, size);
    if (size > transport::kMaxStringBytes) {
        throw transport::ProtocolError("string exceeds the protocol limit");
    }
    out.resize(size);
    if (!out.empty()) {
        transport::read_exact(socket, out.data(), out.size());
    }
}

} // namespace tetris
