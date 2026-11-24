#pragma once
#include "core/shared_types.hpp"
#include <concepts>
#include <optional>

namespace mtlpm::single_tenant
{
/**
 * A route maps a prefix to a next hop. Prefixes are equal when their lengths and normalised bits
 * are equal. A prefix covers an address when their first `length` bits match; /0 covers every
 * address.
 *
 * constructor(routes) Holds exactly the given routes; for a repeated prefix the last one wins.
 * update(prefix, hop) Sets the next hop of exactly this prefix, inserting the route if absent.
 * remove(prefix)      Removes the route of exactly this prefix; a no-op if absent.
 * findNextHop(addr)   The next hop of the longest covering prefix; nullopt if none. Never throws.
 */
template <typename T>
concept SingleTenantForwardingTable =
    std::constructible_from<T, const TenantRoutes &> &&
    requires(T aTable, const T aConstTable, Prefix aPrefix, Address anAddress, NextHop aNextHop) {
        aTable.update(aPrefix, aNextHop);
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
