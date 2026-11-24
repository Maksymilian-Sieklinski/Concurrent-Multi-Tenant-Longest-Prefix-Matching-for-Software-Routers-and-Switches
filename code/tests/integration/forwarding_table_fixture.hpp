#pragma once
#include "core/shared_types.hpp"
#include "forwarding_tables/multi_tenant/multi_tenant_forwarding_table.hpp"
#include <gtest/gtest.h>

namespace mtlpm::tests
{

template <multi_tenant::MultiTenantForwardingTable Table>
class ForwardingTableTest : public ::testing::Test
{
  protected:
    Table theTenSlotTable{TenantCapacity{10}};
};
} // namespace mtlpm::tests