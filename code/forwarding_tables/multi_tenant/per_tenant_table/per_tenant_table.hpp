#pragma once
#include "core/shared_types.hpp"
#include "forwarding_tables/multi_tenant/multi_tenant_forwarding_table.hpp"
#include "forwarding_tables/single_tenant/single_tenant_forwarding_table.hpp"
#include <format>
#include <optional>
#include <stdexcept>
#include <unordered_map>

namespace mtlpm::multi_tenant::per_tenant_table
{
template <single_tenant::SingleTenantForwardingTable Table>
class PerTenantTable
{
  public:
    auto update(TenantId aTenantId, Prefix aPrefix, NextHop aNextHop) -> void
    {
        if (!doesTenantExist(aTenantId))
            throw std::invalid_argument(
                std::format("Tenant with id {} doesn't exists", aTenantId.value()));
        theTenantTables.at(aTenantId).update(aPrefix, aNextHop);
    }

    auto remove(TenantId aTenantId, Prefix aPrefix) -> void
    {
        if (!doesTenantExist(aTenantId))
            throw std::invalid_argument(
                std::format("Tenant with id {} doesn't exists", aTenantId.value()));
        theTenantTables.at(aTenantId).remove(aPrefix);
    }

    auto removeTenant(TenantId aTenantId) -> void
    {
        if (!doesTenantExist(aTenantId))
            throw std::invalid_argument(
                std::format("Tenant with id {} doesn't exists", aTenantId.value()));
        theTenantTables.erase(aTenantId);
    }

    auto addTenant(TenantId aTenantId, const TenantRoutes &aTenantRoutes) -> void
    {
        if (doesTenantExist(aTenantId))
            throw std::invalid_argument(
                std::format("Tenant with id {} already exists", aTenantId.value()));
        theTenantTables.try_emplace(aTenantId, aTenantRoutes);
    }

    [[nodiscard]] auto findNextHop(TenantId aTenantId, Address anAddress) const noexcept
        -> std::optional<NextHop>
    {
        if (!doesTenantExist(aTenantId))
            return std::nullopt;
        return theTenantTables.at(aTenantId).findNextHop(anAddress);
    }

  private:
    [[nodiscard]] auto doesTenantExist(TenantId aTenantId) const -> bool
    {
        return theTenantTables.contains(aTenantId);
    }

    std::unordered_map<TenantId, Table> theTenantTables;
};

static_assert(MultiTenantForwardingTable<
              PerTenantTable<single_tenant::SingleTenantForwardingTableArchetype>>);
} // namespace mtlpm::multi_tenant::per_tenant_table
