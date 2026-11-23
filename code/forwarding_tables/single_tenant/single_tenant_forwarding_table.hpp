#pragma once
#include "core/shared_types.hpp"
#include <concepts>
#include <optional>

namespace mtlpm::single_tenant
{
template <typename T>
concept SingleTenantForwardingTable =
    std::constructible_from<T, const TenantRoutes &> &&
    requires(T aTable, const T aConstTable, Prefix aPrefix, Address anAddress, NextHop aNextHop) {
        // overwrites and inserts if missing a new prefix with a next-hop
        aTable.update(aPrefix, aNextHop);
        // removes a prefix, if doesn't exists a no-op
        aTable.remove(aPrefix);

        { aConstTable.findNextHop(anAddress) } noexcept -> std::same_as<std::optional<NextHop>>;
    };

struct SingleTenantForwardingTableArchetype
{
    explicit SingleTenantForwardingTableArchetype(const TenantRoutes &);
    SingleTenantForwardingTableArchetype(const SingleTenantForwardingTableArchetype &) = delete;
    SingleTenantForwardingTableArchetype(SingleTenantForwardingTableArchetype &&) = delete;
    auto operator=(const SingleTenantForwardingTableArchetype &)
        -> SingleTenantForwardingTableArchetype & = delete;
    auto operator=(SingleTenantForwardingTableArchetype &&)
        -> SingleTenantForwardingTableArchetype & = delete;
    ~SingleTenantForwardingTableArchetype();

    auto update(Prefix, NextHop) -> void;
    auto remove(Prefix) -> void;
    [[nodiscard]] auto findNextHop(Address) const noexcept -> std::optional<NextHop>;
};

static_assert(SingleTenantForwardingTable<SingleTenantForwardingTableArchetype>);
} // namespace mtlpm::single_tenant
