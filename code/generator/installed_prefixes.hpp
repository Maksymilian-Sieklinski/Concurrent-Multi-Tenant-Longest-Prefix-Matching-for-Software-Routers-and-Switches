#pragma once
#include "core/prefix.hpp"
#include "core/shared_types.hpp"
#include "generator/engine.hpp"
#include "generator/operations.hpp"
#include "utils/random_access_set.hpp"

namespace mtlpm::generator
{

class InstalledPrefixes
{
  public:
    explicit InstalledPrefixes(TenantCapacity aTenantCapacity);

    auto addTenant(TenantId aTenantId, const TenantRoutes &aTenantRoutes) -> void;

    [[nodiscard]] auto getRandomPresentTenant(Engine &anEngine) const -> TenantId;

    [[nodiscard]] auto getRandomAbsentTenant(Engine &anEngine) const -> TenantId;

    [[nodiscard]] auto containsTenant(TenantId aTenantId) const -> bool;

    [[nodiscard]] auto tenantContainsPrefix(TenantId aTenantId, Prefix aPrefix) const -> bool;

    [[nodiscard]] auto doesTenantHaveAnyPrefixes(TenantId aTenantId) const -> bool;

    [[nodiscard]] auto getRandomTenantPrefix(TenantId aTenantId, Engine &anEngine) const -> Prefix;

    auto applyOperation(const Operation &anOperation) -> void;

  private:
    auto removeTenant(TenantId aTenantId) -> void;

    auto updatePrefix(TenantId aTenantId, Prefix aPrefix) -> void;

    auto removePrefix(TenantId aTenantId, Prefix aPrefix) -> void;

    auto throwIfOutsideTenantCapacity(TenantId aTenantId) const -> void;

    auto throwOnAbsentOrOutOfCapacityTenant(TenantId aTenantId) const -> void;

    utils::RandomAccessSet<TenantId> thePresentTenants;
    utils::RandomAccessSet<TenantId> theAbsentTenants;
    std::vector<utils::RandomAccessSet<Prefix>> theTenantsPrefixes;
};

} // namespace mtlpm::generator