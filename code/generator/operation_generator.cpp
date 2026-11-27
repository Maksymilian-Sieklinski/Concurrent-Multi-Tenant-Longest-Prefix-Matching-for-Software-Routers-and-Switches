#include "generator/operation_generator.hpp"
#include <cstdint>
#include <stdexcept>

namespace mtlpm::generator
{

namespace
{
auto percentageToProbability(std::uint8_t aPercentage) -> double
{
    return static_cast<double>(aPercentage) / MAX_PERCENTAGE;
}

auto operationKindDistribution(const OperationMix &anOperationMix)
    -> std::discrete_distribution<unsigned>
{
    const auto [myLookupWeight, myUpdateWeight, myRemoveWeight] = anOperationMix;
    return std::discrete_distribution<unsigned>{
        static_cast<double>(myLookupWeight),
        static_cast<double>(myUpdateWeight),
        static_cast<double>(myRemoveWeight),
    };
}
} // namespace

OperationGenerator::OperationGenerator(const ValidatedWorkloadParameters &aWorkloadParameters,
                                       Sampler &aSampler, InstalledPrefixes &anInstalledPrefixes)
    : theSampler(aSampler), theInstalledPrefixes(anInstalledPrefixes),
      theOperationKindDistribution(operationKindDistribution(
          aWorkloadParameters.get().theOperationParameters.theOperationMix)),
      theNewPrefixInsertionDistribution(percentageToProbability(
          aWorkloadParameters.get()
              .theOperationParameters.theNewPrefixInsertionProbabilityInPercentage)),
      theAbsentPrefixRemovalDistribution(percentageToProbability(
          aWorkloadParameters.get()
              .theOperationParameters.theRemovalOfAbsentPrefixProbabilityInPercentage))
{
}

auto OperationGenerator::next() -> Operation
{
    const auto myOperation = createOperation();
    theInstalledPrefixes.applyOperation(myOperation);
    return myOperation;
}

auto OperationGenerator::getRandomOperationKind() -> OperationKind
{
    return static_cast<OperationKind>(theOperationKindDistribution(theSampler.engine()));
}

auto OperationGenerator::createOperation() -> Operation
{
    switch (getRandomOperationKind())
    {
    case OperationKind::Lookup:
        return createLookup();
    case OperationKind::Update:
        return createUpdate();
    case OperationKind::Remove:
        return createRemove();
    }
    throw std::logic_error("Operation kind outside the operation mix");
}

auto OperationGenerator::createLookup() -> Lookup
{
    const auto myTenantId = theInstalledPrefixes.getRandomPresentTenant(theSampler.engine());
    const auto myAddress = theSampler.getRandomAddress();
    return Lookup{.theTenantId = myTenantId, .theAddress = myAddress};
}

auto OperationGenerator::createUpdate() -> Update
{
    const auto myTenantId = theInstalledPrefixes.getRandomPresentTenant(theSampler.engine());
    const auto myInsertsNewPrefix = theNewPrefixInsertionDistribution(theSampler.engine());
    const auto myPrefix = getSampledOrInstalledPrefix(myTenantId, myInsertsNewPrefix);
    const auto myNextHop = theSampler.getRandomNextHop();
    return Update{.theTenantId = myTenantId, .thePrefix = myPrefix, .theNextHop = myNextHop};
}

auto OperationGenerator::createRemove() -> Remove
{
    const auto myTenantId = theInstalledPrefixes.getRandomPresentTenant(theSampler.engine());
    const auto myRemovesAbsentPrefix = theAbsentPrefixRemovalDistribution(theSampler.engine());
    const auto myPrefix = getSampledOrInstalledPrefix(myTenantId, myRemovesAbsentPrefix);
    return Remove{.theTenantId = myTenantId, .thePrefix = myPrefix};
}

auto OperationGenerator::getSampledOrInstalledPrefix(TenantId aTenantId, bool aPrefersSampled)
    -> Prefix
{
    if (aPrefersSampled || !theInstalledPrefixes.doesTenantHaveAnyPrefixes(aTenantId))
        return theSampler.getRandomPrefix();
    return theInstalledPrefixes.getRandomTenantPrefix(aTenantId, theSampler.engine());
}

} // namespace mtlpm::generator
