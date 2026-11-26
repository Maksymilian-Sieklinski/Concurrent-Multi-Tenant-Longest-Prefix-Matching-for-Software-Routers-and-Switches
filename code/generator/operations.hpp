#pragma once
#include "core/shared_types.hpp"
#include <type_traits>
#include <variant>

namespace mtlpm::generator
{

struct Lookup
{
    TenantId theTenantId;
    Address theAddress;
};
static_assert(std::is_aggregate<Lookup>());

struct Update
{
    TenantId theTenantId;
    Prefix thePrefix;
    NextHop theNextHop;
};
static_assert(std::is_aggregate<Update>());

struct Remove
{
    TenantId theTenantId;
    Prefix thePrefix;
};
static_assert(std::is_aggregate<Remove>());

using Operation = std::variant<Lookup, Update, Remove>;
} // namespace mtlpm::generator