#pragma once
#include "utils/hashable.hpp"
#include <compare> // IWYU pragma: keep
#include <ostream>

namespace utils
{

template <typename T, typename Tag>
class Tagged
{
  public:
    constexpr explicit Tagged(T aValue) noexcept : theValue(aValue) {}
    [[nodiscard]] constexpr auto value() const noexcept -> T { return theValue; }
    constexpr auto operator<=>(const Tagged &) const noexcept = default;

    friend auto operator<<(std::ostream &anOutput, const Tagged &aTagged) -> std::ostream &
    {
        // + is integral promotion for uint8_t and uint16_t, so we don't print char
        return anOutput << +aTagged.value();
    }

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