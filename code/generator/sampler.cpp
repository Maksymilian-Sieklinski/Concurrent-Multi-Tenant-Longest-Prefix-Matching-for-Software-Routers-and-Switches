#include "generator/sampler.hpp"
#include <limits>

namespace mtlpm::generator
{

Sampler::Sampler(const ValidatedWorkloadParameters &aWorkloadParameters, Seed aSeed)
    : theEngine(aSeed.value()),
      theAddressDistribution(0, std::numeric_limits<std::uint32_t>::max()),
      thePrefixLengthDistribution(aWorkloadParameters.get().thePrefixesParameters.theMinLength,
                                  aWorkloadParameters.get().thePrefixesParameters.theMaxLength),
      theNextHopDistribution(
          0, aWorkloadParameters.get().theTenantsParameters.theNextHopsPerTenant - 1)
{
}

auto Sampler::getRandomAddress() -> Address { return Address{theAddressDistribution(theEngine)}; }

auto Sampler::getRandomPrefix() -> Prefix
{
    const auto myBits = theAddressDistribution(theEngine);
    const auto myLength = static_cast<std::uint8_t>(thePrefixLengthDistribution(theEngine));
    return Prefix{myBits, myLength};
}

auto Sampler::getRandomNextHop() -> NextHop { return NextHop{theNextHopDistribution(theEngine)}; }

auto Sampler::engine() noexcept -> Engine & { return theEngine; }

} // namespace mtlpm::generator
