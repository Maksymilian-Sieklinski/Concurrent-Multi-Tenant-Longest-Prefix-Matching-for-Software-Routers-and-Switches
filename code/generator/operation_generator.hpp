#pragma once
#include "core/prefix.hpp"
#include "core/shared_types.hpp"
#include "generator/installed_prefixes.hpp"
#include "generator/operations.hpp"
#include "generator/sampler.hpp"
#include "generator/validated_workload_parameters.hpp"
#include <random>

namespace mtlpm::generator
{

class OperationGenerator
{
  public:
    OperationGenerator(const ValidatedWorkloadParameters &aWorkloadParameters, Sampler &aSampler,
                       InstalledPrefixes &anInstalledPrefixes);

    [[nodiscard]] auto next() -> Operation;

  private:
    [[nodiscard]] auto getRandomOperationKind() -> OperationKind;

    [[nodiscard]] auto createOperation() -> Operation;

    [[nodiscard]] auto createLookup() -> Lookup;

    [[nodiscard]] auto createUpdate() -> Update;

    [[nodiscard]] auto createRemove() -> Remove;

    [[nodiscard]] auto getSampledOrInstalledPrefix(TenantId aTenantId, bool aPrefersSampled)
        -> Prefix;

    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    Sampler &theSampler;
    InstalledPrefixes &theInstalledPrefixes;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
    std::discrete_distribution<unsigned> theOperationKindDistribution;
    std::bernoulli_distribution theNewPrefixInsertionDistribution;
    std::bernoulli_distribution theAbsentPrefixRemovalDistribution;
};

} // namespace mtlpm::generator
