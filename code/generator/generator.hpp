#pragma once
#include "core/shared_types.hpp"
#include "generator/installed_prefixes.hpp"
#include "generator/operations.hpp"
#include "generator/sampler.hpp"
#include "generator/validated_workload_parameters.hpp"
#include "generator/workload.hpp"
#include <vector>

namespace mtlpm::generator
{

class Generator
{
  public:
    explicit Generator(const ValidatedWorkloadParameters &aWorkloadParameters)
        : theWorkloadParameters(aWorkloadParameters)
    {
    }

    [[nodiscard]] auto generateWorkload(Seed aSeed) const -> Workload;

  private:
    [[nodiscard]] auto createInitialTenants(Sampler &aSampler,
                                            InstalledPrefixes &anInstalledPrefixes) const
        -> std::vector<TenantRoutes>;

    [[nodiscard]] auto createTenantRoutes(Sampler &aSampler) const -> TenantRoutes;

    [[nodiscard]] auto createOperations(Sampler &aSampler,
                                        InstalledPrefixes &anInstalledPrefixes) const
        -> std::vector<Operation>;

    ValidatedWorkloadParameters theWorkloadParameters;
};

} // namespace mtlpm::generator
