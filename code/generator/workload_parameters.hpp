#pragma once

#include "generator/operations.hpp"
#include <cstdint>
#include <type_traits>
namespace mtlpm::generator
{

struct TenantsParameters
{
    std::uint32_t theCapacity = 10;
    std::uint32_t theInitialCount = 10;
    std::uint32_t theRoutesPerTenant = 100;
    std::uint16_t theNextHopsPerTenant = 16;
};
static_assert(std::is_aggregate<TenantsParameters>());

struct PrefixesParameters
{
    std::uint8_t theMinLength = 16;
    std::uint8_t theMaxLength = 24;
};
static_assert(std::is_aggregate<PrefixesParameters>());

struct OperationMix
{
    std::uint32_t theLookupWeight = 90;
    std::uint32_t theUpdateWeight = 8;
    std::uint32_t theRemoveWeight = 2;
};
// ensure that every operation has its field
static_assert(std::variant_size_v<Operation> == sizeof(OperationMix) / sizeof(std::uint32_t));
static_assert(std::is_aggregate<OperationMix>());

struct OperationParameters
{
    std::uint32_t theNumberOfOperations = 1000;
    std::uint8_t theNewPrefixInsertionProbabilityInPercentage = 50;
    std::uint8_t theRemovalOfAbsentPrefixProbabilityInPercentage = 10;
    OperationMix theOperationMix;
};
static_assert(std::is_aggregate<OperationParameters>());

struct WorkloadParameters
{
    TenantsParameters theTenantsParameters;
    PrefixesParameters thePrefixesParameters;
    OperationParameters theOperationParameters;
};
static_assert(std::is_aggregate<WorkloadParameters>());

} // namespace mtlpm::generator
