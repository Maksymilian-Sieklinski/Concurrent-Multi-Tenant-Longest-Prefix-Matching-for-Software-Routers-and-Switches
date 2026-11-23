#pragma once
#include "core/shared_types.hpp"
#include <concepts>
#include <optional>

namespace mtlpm::multi_tenant
{
template <typename T>
concept MultiTenantForwardingTable =
    std::default_initializable<T> &&
    requires(T aTable, const T aConstTable, TenantId aTenantId, Prefix aPrefix, Address anAddress,
             NextHop aNextHop, TenantRoutes aTenantRoutes) {
        // overwrites and inserts if missing a new prefix with a next-hop
        aTable.update(aTenantId, aPrefix, aNextHop);
        // removes a prefix, if doesn't exists a no-op
        aTable.remove(aTenantId, aPrefix);

        aTable.removeTenant(aTenantId);
        aTable.addTenant(aTenantId, aTenantRoutes);

        {
            aConstTable.findNextHop(aTenantId, anAddress)
        } noexcept -> std::same_as<std::optional<NextHop>>;
    };
} // namespace mtlpm::multi_tenant