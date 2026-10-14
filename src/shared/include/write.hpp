#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <concept_simple.hpp>
#include <transport.hpp>

namespace tetris {

void ensure_write(int s, const void *ptr, size_t amt);

namespace detail {

template <typename T>
void encode_simple(T const &value, std::array<unsigned char, sizeof(std::uint64_t)> &bytes) {
    using value_type = std::remove_cv_t<T>;
    using logical_type = transport::detail::wire_logical_t<value_type>;
    using unsigned_type = transport::detail::wire_unsigned_t<logical_type>;

    std::uint64_t raw = 0;
    if constexpr (std::is_same_v<value_type, std::size_t>) {
        raw = static_cast<std::uint64_t>(value);
    } else if constexpr (std::is_same_v<logical_type, bool>) {
        raw = value ? 1u : 0u;
    } else {
        logical_type logical = static_cast<logical_type>(value);
        unsigned_type bits{};
        static_assert(sizeof(bits) <= sizeof(raw));
        std::memcpy(&bits, &logical, sizeof(bits));
        raw = static_cast<std::uint64_t>(bits);
    }

    constexpr std::size_t width = transport::detail::wire_size_v<value_type>;
    for (std::size_t i = 0; i < width; ++i) {
        bytes[width - i - 1] = static_cast<unsigned char>(raw & 0xffu);
        raw >>= 8u;
    }
}

} // namespace detail

template <typename T>
    requires simple<T>
inline void write(int s, T const &i) {
    std::array<unsigned char, sizeof(std::uint64_t)> bytes{};
    detail::encode_simple(i, bytes);
    ensure_write(s, bytes.data(), transport::detail::wire_size_v<T>);
};

inline void write(int s, std::string const &str) {
    if (str.size() > transport::kMaxStringBytes) {
        throw transport::ProtocolError("string exceeds the protocol limit");
    }
    write(s, str.size());
    if (!str.empty()) {
        ensure_write(s, str.data(), str.size());
    }
};

template <typename T> inline void write(int s, std::vector<T> const &vec) {
    if (vec.size() > transport::kMaxVectorElements) {
        throw transport::ProtocolError("vector exceeds the protocol limit");
    }
    write(s, vec.size());
    for (auto const &i : vec) {
        write(s, i);
    }
};
} // namespace tetris
