#pragma once
#include "core/shared_types.hpp"
#include "generator/operations.hpp"
#include <vector>

namespace mtlpm::generator
{

struct Workload
{
    TenantCapacity theTenantCapacity;
    std::vector<TenantRoutes> theInitialTenants;
    std::vector<Operation> theOperations;
};

} // namespace mtlpm::generator