#include "generator/generator.hpp"
#include "generator/operation_generator.hpp"
#include <cstdint>
#include <ranges>
#include <utility>

namespace mtlpm::generator
{

auto Generator::generateWorkload(Seed aSeed) const -> Workload
{
    Sampler mySampler{theWorkloadParameters, aSeed};
    const TenantCapacity myTenantCapacity{
        theWorkloadParameters.get().theTenantsParameters.theCapacity};
    InstalledPrefixes myInstalledPrefixes{myTenantCapacity};
    auto myInitialTenants = createInitialTenants(mySampler, myInstalledPrefixes);
    auto myOperations = createOperations(mySampler, myInstalledPrefixes);
    return Workload{
        .theTenantCapacity = myTenantCapacity,
        .theInitialTenants = std::move(myInitialTenants),
        .theOperations = std::move(myOperations),
    };
}

auto Generator::createInitialTenants(Sampler &aSampler,
                                     InstalledPrefixes &anInstalledPrefixes) const
    -> std::vector<TenantRoutes>
{
    const auto myNumberOfTenants = theWorkloadParameters.get().theTenantsParameters.theInitialCount;
    std::vector<TenantRoutes> myInitialTenants;
    myInitialTenants.reserve(myNumberOfTenants);
    for (const auto myTenantId : std::views::iota(std::uint32_t{0}, myNumberOfTenants))
    {
        const auto &myTenantRoutes = myInitialTenants.emplace_back(createTenantRoutes(aSampler));
        anInstalledPrefixes.addTenant(TenantId{myTenantId}, myTenantRoutes);
    }
    return myInitialTenants;
}

auto Generator::createTenantRoutes(Sampler &aSampler) const -> TenantRoutes
{
    const auto myNumberOfRoutes =
        theWorkloadParameters.get().theTenantsParameters.theRoutesPerTenant;
    TenantRoutes myTenantRoutes;
    myTenantRoutes.theRoutes.reserve(myNumberOfRoutes);
    for (std::uint32_t myRouteIndex = 0; myRouteIndex < myNumberOfRoutes; ++myRouteIndex)
    {
        const auto myPrefix = aSampler.getRandomPrefix();
        const auto myNextHop = aSampler.getRandomNextHop();
        myTenantRoutes.theRoutes.emplace_back(myPrefix, myNextHop);
    }
    return myTenantRoutes;
}

auto Generator::createOperations(Sampler &aSampler, InstalledPrefixes &anInstalledPrefixes) const
    -> std::vector<Operation>
{
    const auto myNumberOfOperations =
        theWorkloadParameters.get().theOperationParameters.theNumberOfOperations;
    OperationGenerator myOperationGenerator{theWorkloadParameters, aSampler, anInstalledPrefixes};
    std::vector<Operation> myOperations;
    myOperations.reserve(myNumberOfOperations);
    for (std::uint32_t myOperationIndex = 0; myOperationIndex < myNumberOfOperations;
         ++myOperationIndex)
        myOperations.emplace_back(myOperationGenerator.next());
    return myOperations;
}

} // namespace mtlpm::generator
