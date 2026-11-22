#pragma once
#include "utils/tagged.hpp"
#include <cstdint>

using NextHop = Tagged<std::uint16_t, struct NextHopTag>;
using TenantId = Tagged<std::uint32_t, struct TenantIdTag>;
using Address = Tagged<std::uint32_t, struct AddressTag>; // IPv4

struct Prefix
{
    Address theAddress;
    std::uint8_t thePrefixLength;
};