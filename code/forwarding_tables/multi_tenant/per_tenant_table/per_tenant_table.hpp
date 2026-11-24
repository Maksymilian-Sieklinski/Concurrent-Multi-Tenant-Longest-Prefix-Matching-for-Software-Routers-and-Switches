#pragma once
#include "core/shared_types.hpp"
#include "forwarding_tables/multi_tenant/multi_tenant_forwarding_table.hpp"
#include "forwarding_tables/single_tenant/single_tenant_forwarding_table.hpp"
#include <format>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

namespace mtlpm::multi_tenant::per_tenant_table
{
template <single_tenant::SingleTenantForwardingTable Table>
class PerTenantTable
{
  public:
    explicit PerTenantTable(TenantCapacity aTenantCapacity)
        : theTenantTables(aTenantCapacity.value())
    {
    }

    auto update(TenantId aTenantId, Prefix aPrefix, NextHop aNextHop) -> void
    {
        getExistingTenantSlotOrThrow(aTenantId)->update(aPrefix, aNextHop);
    }

    auto remove(TenantId aTenantId, Prefix aPrefix) -> void
    {
        getExistingTenantSlotOrThrow(aTenantId)->remove(aPrefix);
    }

    auto removeTenant(TenantId aTenantId) -> void
    {
        getExistingTenantSlotOrThrow(aTenantId) = nullptr;
    }

    auto addTenant(TenantId aTenantId, const TenantRoutes &aTenantRoutes) -> void
    {
        throwOnOutsideTenantBounds(aTenantId);
        if (doesTenantExist(aTenantId))
            throw std::invalid_argument(
                std::format("Tenant with id {} already exists", aTenantId.value()));
        theTenantTables[aTenantId.value()] = std::make_unique<Table>(aTenantRoutes);
    }

    [[nodiscard]] auto findNextHop(TenantId aTenantId, Address anAddress) const noexcept
        -> std::optional<NextHop>
    {
        if (isOutsideTenantBounds(aTenantId) || !doesTenantExist(aTenantId))
            return std::nullopt;
        return theTenantTables[aTenantId.value()]->findNextHop(anAddress);
    }

  private:
    [[nodiscard]] auto isOutsideTenantBounds(TenantId aTenantId) const noexcept -> bool
    {
        return aTenantId.value() >= theTenantTables.size();
    }

    [[nodiscard]] auto doesTenantExist(TenantId aTenantId) const noexcept -> bool
    {
        return theTenantTables[aTenantId.value()] != nullptr;
    }

    auto throwOnOutsideTenantBounds(TenantId aTenantId) const -> void
    {
        if (isOutsideTenantBounds(aTenantId))
            throw std::invalid_argument(std::format(
                "Tenant with id {} is outside defined tenant capacity", aTenantId.value()));
    }

    [[nodiscard]] auto getExistingTenantSlotOrThrow(TenantId aTenantId) -> std::unique_ptr<Table> &
    {
        throwOnOutsideTenantBounds(aTenantId);
        auto &myTenantTable = theTenantTables[aTenantId.value()];
        if (myTenantTable == nullptr)
            throw std::invalid_argument(
                std::format("Tenant with id {} doesn't exists", aTenantId.value()));
        return myTenantTable;
    }

    std::vector<std::unique_ptr<Table>> theTenantTables;
};

static_assert(MultiTenantForwardingTable<
              PerTenantTable<single_tenant::SingleTenantForwardingTableArchetype>>);
} // namespace mtlpm::multi_tenant::per_tenant_table
