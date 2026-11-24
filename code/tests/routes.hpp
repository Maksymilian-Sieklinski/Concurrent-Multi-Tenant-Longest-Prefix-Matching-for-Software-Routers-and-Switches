#pragma once
#include "core/shared_types.hpp"
#include <concepts>

namespace mtlpm::tests
{
template <std::same_as<Route>... Routes>
auto routes(Routes... aRoutes) -> TenantRoutes
{
    return TenantRoutes{{aRoutes...}};
}
} // namespace mtlpm::tests