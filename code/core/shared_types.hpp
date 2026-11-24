#pragma once
#include "core/prefix.hpp"
#include "utils/tagged.hpp"
#include <cstdint>
#include <vector>

namespace mtlpm
{

using NextHop = utils::Tagged<std::uint16_t, struct NextHopTag>;
using TenantId = utils::Tagged<std::uint32_t, struct TenantIdTag>;
using Address = utils::Tagged<std::uint32_t, struct AddressTag>; // IPv4

struct Route
{
    Prefix thePrefix;
    NextHop theNextHop;
};

struct TenantRoutes
{
    std::vector<Route> theRoutes;
};
} // namespace mtlpm