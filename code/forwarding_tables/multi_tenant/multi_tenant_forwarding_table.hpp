#pragma once
#include "core/shared_types.hpp"
#include <concepts>
#include <optional>

namespace mtlpm::multi_tenant
{
/**
 * Starts with no tenants. Any TenantId is valid. Within a tenant, routes behave as in
 * SingleTenantForwardingTable; operations on one tenant never affect another. A call that throws
 * leaves the table unchanged.
 *
 * addTenant(id, routes) Adds a tenant holding exactly the given routes. Throws
 *                       std::invalid_argument if the tenant exists.
 * removeTenant(id)      Removes the tenant and all its routes. Throws std::invalid_argument if
 *                       the tenant does not exist.
 * update, remove        As in SingleTenantForwardingTable, on the tenant's routes. Throw
 *                       std::invalid_argument if the tenant does not exist.
 * findNextHop(id, addr) As in SingleTenantForwardingTable; nullopt if the tenant does not exist.
 *                       Never throws.
 */
template <typename T>
concept MultiTenantForwardingTable =
    std::default_initializable<T> &&
    requires(T aTable, const T aConstTable, TenantId aTenantId, Prefix aPrefix, Address anAddress,
             NextHop aNextHop, TenantRoutes aTenantRoutes) {
        aTable.update(aTenantId, aPrefix, aNextHop);
        aTable.remove(aTenantId, aPrefix);

        aTable.removeTenant(aTenantId);
        aTable.addTenant(aTenantId, aTenantRoutes);

        {
            aConstTable.findNextHop(aTenantId, anAddress)
        } noexcept -> std::same_as<std::optional<NextHop>>;
    };
} // namespace mtlpm::multi_tenant