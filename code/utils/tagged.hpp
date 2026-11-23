#pragma once
#include <compare> // IWYU pragma: keep
#include <concepts>
#include <functional>

namespace utils
{
template <typename T>
concept Hashable = requires(const T &aValue) {
    { std::hash<T>{}(aValue) } -> std::convertible_to<size_t>;
};

template <typename T, typename Tag>
class Tagged
{
  public:
    constexpr explicit Tagged(T aValue) noexcept : theValue(aValue) {}
    [[nodiscard]] constexpr auto value() const noexcept -> T { return theValue; }
    constexpr auto operator<=>(const Tagged &) const noexcept = default;

  private:
    T theValue;
};
}; // namespace utils

template <utils::Hashable T, typename Tag>
struct std::hash<utils::Tagged<T, Tag>>
{
    auto operator()(const utils::Tagged<T, Tag> &aTagged) const noexcept -> std::size_t
    {
        return std::hash<T>{}(aTagged.value());
    }
};