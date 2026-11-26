#pragma once
#include <concepts>
#include <cstddef>
#include <functional>

namespace utils
{
template <typename T>
concept Hashable = requires(const T &aValue) {
    { std::hash<T>{}(aValue) } -> std::convertible_to<std::size_t>;
};
} // namespace utils