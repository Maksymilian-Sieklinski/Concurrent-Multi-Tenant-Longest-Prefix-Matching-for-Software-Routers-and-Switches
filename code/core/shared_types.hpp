#pragma once
#include "core/prefix.hpp"
#include "utils/tagged.hpp"
#include <cstdint>
#include <vector>

namespace mtlpm
{

using NextHop = utils::Tagged<std::uint16_t, struct NextHopTag>;
static_assert(sizeof(NextHop) == sizeof(std::uint16_t));

using TenantId = utils::Tagged<std::uint32_t, struct TenantIdTag>;
static_assert(sizeof(TenantId) == sizeof(std::uint32_t));

using Address = utils::Tagged<std::uint32_t, struct AddressTag>; // IPv4
static_assert(sizeof(Address) == sizeof(std::uint32_t));

using TenantCapacity = utils::Tagged<std::uint32_t, struct TenantCapacityTag>;
static_assert(sizeof(TenantCapacity) == sizeof(std::uint32_t));

using Seed = utils::Tagged<std::uint64_t, struct SeedTag>;
static_assert(sizeof(Seed) == sizeof(std::uint64_t));

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