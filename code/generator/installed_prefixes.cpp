#include "generator/installed_prefixes.hpp"
#include "core/shared_types.hpp"
#include "generator/operations.hpp"
#include "utils/overload.hpp"
#include <ranges>
#include <stdexcept>
#include <variant>

namespace mtlpm::generator
{

InstalledPrefixes::InstalledPrefixes(TenantCapacity aTenantCapacity,
                                     const std::vector<TenantRoutes> &aInitialTenants)

{
    if (aInitialTenants.size() > aTenantCapacity.value())
        throw std::logic_error("Number of initial tenants exceeds the tenant capacity");

    theTenantsPrefixes.resize(aTenantCapacity.value());
    for (const auto myTenantId : std::views::iota(
             static_cast<std::uint32_t>(aInitialTenants.size()), aTenantCapacity.value()))
        theAbsentTenants.insert(TenantId{myTenantId});

    for (const auto &[myTenantId, myTenantRoues] : std::ranges::views::enumerate(aInitialTenants))
        addTenant(TenantId{static_cast<std::uint32_t>(myTenantId)}, myTenantRoues);
}

auto InstalledPrefixes::getRandomPresentTenant(Engine &anEngine) const -> TenantId
{
    return thePresentTenants.getRandom(anEngine);
}

auto InstalledPrefixes::getRandomAbsentTenant(Engine &anEngine) const -> TenantId
{
    return theAbsentTenants.getRandom(anEngine);
}

auto InstalledPrefixes::contains(TenantId aTenantId) const -> bool
{
    return thePresentTenants.contains(aTenantId);
}

auto InstalledPrefixes::contains(TenantId aTenantId, Prefix aPrefix) const -> bool
{
    throwOnAbsentOrOutOfCapacityTenant(aTenantId);
    return theTenantsPrefixes[aTenantId.value()].contains(aPrefix);
}

auto InstalledPrefixes::applyOperation(const Operation &anOperation) -> void
{
    auto myOverload = utils::Overload{
        [&](const Update &anUpdate) -> void
        { updatePrefix(anUpdate.theTenantId, anUpdate.thePrefix); },
        [&](const Remove &aRemove) -> void
        { removePrefix(aRemove.theTenantId, aRemove.thePrefix); },
        [](const Lookup &) {},
    };
    std::visit(myOverload, anOperation);
}

auto InstalledPrefixes::addTenant(TenantId aTenantId, const TenantRoutes &aTenantRoutes) -> void
{
    throwIfOutsideTenantCapacity(aTenantId);
    if (thePresentTenants.contains(aTenantId))
        throw std::logic_error(std::format("Tenant with id {} already exists", aTenantId.value()));

    theAbsentTenants.erase(aTenantId);
    thePresentTenants.insert(aTenantId);

    auto &myPrefixSet = theTenantsPrefixes[aTenantId.value()];
    for (const auto &[myPrefix, myNextHop] : aTenantRoutes.theRoutes)
        myPrefixSet.insert(myPrefix);
}

auto InstalledPrefixes::removeTenant(TenantId aTenantId) -> void
{
    throwOnAbsentOrOutOfCapacityTenant(aTenantId);
    thePresentTenants.erase(aTenantId);
    theAbsentTenants.insert(aTenantId);
    theTenantsPrefixes[aTenantId.value()].clear();
}

auto InstalledPrefixes::updatePrefix(TenantId aTenantId, Prefix aPrefix) -> void
{
    throwOnAbsentOrOutOfCapacityTenant(aTenantId);
    theTenantsPrefixes[aTenantId.value()].insert(aPrefix);
}

auto InstalledPrefixes::removePrefix(TenantId aTenantId, Prefix aPrefix) -> void
{
    throwOnAbsentOrOutOfCapacityTenant(aTenantId);
    theTenantsPrefixes[aTenantId.value()].erase(aPrefix);
}

auto InstalledPrefixes::throwIfOutsideTenantCapacity(TenantId aTenantId) const -> void
{
    if (aTenantId.value() >= theTenantsPrefixes.size())
        throw std::logic_error(
            std::format("Tenant with id {} is outside defined tenant capacity", aTenantId.value()));
}

auto InstalledPrefixes::throwOnAbsentOrOutOfCapacityTenant(TenantId aTenantId) const -> void
{
    throwIfOutsideTenantCapacity(aTenantId);
    if (!thePresentTenants.contains(aTenantId))
        throw std::logic_error(std::format("Tenant with id {} doesn't exists", aTenantId.value()));
}

} // namespace mtlpm::generator