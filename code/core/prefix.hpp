#pragma once
#include "core/address_bits.hpp"
#include <cstdint>
#include <format>
#include <stdexcept>

namespace mtlpm
{

class Prefix
{
  public:
    Prefix(std::uint32_t aPrefixBits, std::uint8_t aPrefixLength)
        : thePrefixLength(validate(aPrefixLength)),
          thePrefixBits(normalize(aPrefixBits, aPrefixLength))
    {
    }

    [[nodiscard]] constexpr auto theBits() const noexcept -> std::uint32_t { return thePrefixBits; }
    [[nodiscard]] constexpr auto theLength() const noexcept -> std::uint8_t
    {
        return thePrefixLength;
    }
    auto operator==(const Prefix &) const -> bool = default;

  private:
    static auto validate(std::uint8_t aPrefixLength) -> std::uint8_t
    {
        if (aPrefixLength > ADDRESS_BITS)
            throw std::invalid_argument(std::format("Prefix length {} is longer than {} bits",
                                                    aPrefixLength, ADDRESS_BITS));
        return aPrefixLength;
    }

    static constexpr auto normalize(std::uint32_t aPrefixBits, std::uint8_t aPrefixLength) noexcept
        -> std::uint32_t
    {
        const auto myMask = prefixMask(aPrefixLength);
        return aPrefixBits & myMask;
    }

    std::uint8_t thePrefixLength;
    std::uint32_t thePrefixBits;
};

} // namespace mtlpm

template <>
struct std::hash<mtlpm::Prefix>
{
    auto operator()(const mtlpm::Prefix &aPrefix) const noexcept -> std::size_t
    {
        const auto myPacked =
            (static_cast<std::uint64_t>(aPrefix.theBits()) << 8U) | aPrefix.theLength();
        return std::hash<std::uint64_t>{}(myPacked);
    }
};