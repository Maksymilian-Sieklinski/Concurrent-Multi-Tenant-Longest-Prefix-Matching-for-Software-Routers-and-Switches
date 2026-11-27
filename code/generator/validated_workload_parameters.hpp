#pragma once
#include "core/address_bits.hpp"
#include "generator/workload_parameters.hpp"
#include <cstdint>
#include <format>
#include <stdexcept>

namespace mtlpm::generator
{

inline constexpr std::uint8_t MAX_PERCENTAGE = 100;

class ValidatedWorkloadParameters
{
  public:
    [[nodiscard]] static constexpr auto validate(const WorkloadParameters &aWorkloadParameters)
        -> ValidatedWorkloadParameters
    {
        validateTenants(aWorkloadParameters.theTenantsParameters);
        validatePrefixes(aWorkloadParameters.thePrefixesParameters);
        validateOperations(aWorkloadParameters.theOperationParameters);
        if (aWorkloadParameters.theOperationParameters.theNumberOfOperations > 0 &&
            aWorkloadParameters.theTenantsParameters.theInitialCount == 0)
            throw std::invalid_argument("Operations require at least one initial tenant");
        return ValidatedWorkloadParameters{aWorkloadParameters};
    }

    [[nodiscard]] constexpr auto get() const noexcept -> const WorkloadParameters &
    {
        return theWorkloadParameters;
    }

  private:
    explicit constexpr ValidatedWorkloadParameters(const WorkloadParameters &aWorkloadParameters)
        : theWorkloadParameters(aWorkloadParameters)
    {
    }

    static constexpr auto validateTenants(const TenantsParameters &aTenantsParameters) -> void
    {
        if (aTenantsParameters.theInitialCount > aTenantsParameters.theCapacity)
            throw std::invalid_argument(
                std::format("Initial tenant count {} exceeds tenant capacity {}",
                            aTenantsParameters.theInitialCount, aTenantsParameters.theCapacity));
        if (aTenantsParameters.theNextHopsPerTenant == 0)
            throw std::invalid_argument("Number of next hops per tenant must be at least 1");
    }

    static constexpr auto validatePrefixes(const PrefixesParameters &aPrefixesParameters) -> void
    {
        const auto [myMinLength, myMaxLength] = aPrefixesParameters;
        if (myMinLength > myMaxLength)
            throw std::invalid_argument(
                std::format("Minimum prefix length {} exceeds maximum prefix length {}",
                            myMinLength, myMaxLength));
        if (myMaxLength > ADDRESS_BITS)
            throw std::invalid_argument(std::format(
                "Maximum prefix length {} is longer than {} bits", myMaxLength, ADDRESS_BITS));
    }

    static constexpr auto validateOperations(const OperationParameters &anOperationParameters)
        -> void
    {
        if (anOperationParameters.theNewPrefixInsertionProbabilityInPercentage > MAX_PERCENTAGE)
            throw std::invalid_argument(
                std::format("New prefix insertion probability {}% exceeds 100%",
                            anOperationParameters.theNewPrefixInsertionProbabilityInPercentage));
        if (anOperationParameters.theRemovalOfAbsentPrefixProbabilityInPercentage > MAX_PERCENTAGE)
            throw std::invalid_argument(
                std::format("Removal of absent prefix probability {}% exceeds 100%",
                            anOperationParameters.theRemovalOfAbsentPrefixProbabilityInPercentage));
        const auto [myLookupWeight, myUpdateWeight, myRemoveWeight] =
            anOperationParameters.theOperationMix;
        if (myLookupWeight == 0 && myUpdateWeight == 0 && myRemoveWeight == 0)
            throw std::invalid_argument("At least one operation weight must be positive");
    }

    WorkloadParameters theWorkloadParameters;
};

} // namespace mtlpm::generator
