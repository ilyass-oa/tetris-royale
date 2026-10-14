#pragma once
#include <type_traits>

namespace tetris {
template <typename T>
concept simple = std::is_integral_v<T> || std::is_enum_v<T>;
} // namespace tetris
