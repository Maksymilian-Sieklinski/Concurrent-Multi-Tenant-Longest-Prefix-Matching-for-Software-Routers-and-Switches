#pragma once
#include "core/prefix.hpp"
#include "core/shared_types.hpp"
#include "generator/engine.hpp"
#include "generator/validated_workload_parameters.hpp"
#include <cstdint>
#include <random>

namespace mtlpm::generator
{

class Sampler
{
  public:
    Sampler(const ValidatedWorkloadParameters &aWorkloadParameters, Seed aSeed);

    [[nodiscard]] auto getRandomAddress() -> Address;

    [[nodiscard]] auto getRandomPrefix() -> Prefix;

    [[nodiscard]] auto getRandomNextHop() -> NextHop;

    [[nodiscard]] auto engine() noexcept -> Engine &;

  private:
    Engine theEngine;
    std::uniform_int_distribution<std::uint32_t> theAddressDistribution;
    std::uniform_int_distribution<std::uint32_t> thePrefixLengthDistribution;
    std::uniform_int_distribution<std::uint16_t> theNextHopDistribution;
};

} // namespace mtlpm::generator
