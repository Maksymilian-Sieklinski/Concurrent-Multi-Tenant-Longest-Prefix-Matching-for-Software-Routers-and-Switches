#pragma once
#include <cstdint>

namespace mtlpm
{
inline constexpr std::uint8_t ADDRESS_BITS = 32;

constexpr auto prefixMask(std::uint8_t aPrefixLength) noexcept -> std::uint32_t
{
    const auto mySuffixLength = static_cast<unsigned>(ADDRESS_BITS - aPrefixLength);
    return static_cast<std::uint32_t>(~std::uint64_t{0} << mySuffixLength);
}

constexpr auto addressBit(std::uint32_t aBits, std::uint8_t anIndex) noexcept -> std::uint32_t
{
    return (aBits >> (ADDRESS_BITS - 1U - anIndex)) & 1U;
}

} // namespace mtlpm
