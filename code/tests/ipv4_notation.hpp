#pragma once
#include "core/prefix.hpp"
#include "core/shared_types.hpp"
#include <cstdint>

namespace mtlpm::tests
{

constexpr auto address(std::uint8_t aFirstOctet, std::uint8_t aSecondOctet,
                       std::uint8_t aThirdOctet, std::uint8_t aFourthOctet) -> Address
{
    const std::uint32_t myValue = (std::uint32_t{aFirstOctet} << 24U) |
                                  (std::uint32_t{aSecondOctet} << 16U) |
                                  (std::uint32_t{aThirdOctet} << 8U) | std::uint32_t{aFourthOctet};
    return Address{myValue};
}

constexpr auto prefix(std::uint8_t aFirstOctet, std::uint8_t aSecondOctet, std::uint8_t aThirdOctet,
                      std::uint8_t aFourthOctet, std::uint8_t aPrefixLength) -> Prefix
{
    const auto myBits = address(aFirstOctet, aSecondOctet, aThirdOctet, aFourthOctet).value();
    return Prefix{myBits, aPrefixLength};
}
} // namespace mtlpm::tests