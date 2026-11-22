#pragma once
#include <compare> // IWYU pragma: keep

template <typename T, typename Tag> class Tagged
{
  public:
    constexpr explicit Tagged(T aValue) noexcept : theValue(aValue) {};
    constexpr auto value() const noexcept -> T { return theValue; }
    constexpr auto operator<=>(const Tagged &) const noexcept = default;

  private:
    T theValue;
};