#include "core/shared_types.hpp"
#include "tests/implementations.hpp"
#include "tests/ipv4_notation.hpp"
#include "tests/routes.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <optional>
#include <ranges>
#include <stdexcept>

namespace mtlpm::tests
{
namespace
{
template <typename Table>
auto addEveryTenantWithDefaultRoute(Table &aTable, TenantCapacity aCapacity) -> void
{
    for (const auto myId : std::views::iota(0U, aCapacity.value()))
        aTable.addTenant(TenantId{myId}, routes(Route{
                                             prefix(0, 0, 0, 0, 0),
                                             NextHop{static_cast<std::uint16_t>(myId)},
                                         }));
}
} // namespace

template <typename Table>
class TenantBounds : public ::testing::Test
{
};
TYPED_TEST_SUITE(TenantBounds, MultiTenantImplementations, ImplementationNames);

TYPED_TEST(TenantBounds, FirstAndLastValidTenantsResolveIndependently)
{
    const auto myCapacity = TenantCapacity{4};
    const auto myFirstTenant = TenantId{0};
    const auto myLastTenant = TenantId{myCapacity.value() - 1};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myFirstNextHop = NextHop{1};
    const auto myLastNextHop = NextHop{2};
    const auto myAddress = address(10, 1, 2, 3);

    TypeParam myTable{myCapacity};
    myTable.addTenant(myFirstTenant, routes(Route{myPrefix, myFirstNextHop}));
    myTable.addTenant(myLastTenant, routes(Route{myPrefix, myLastNextHop}));

    EXPECT_EQ(myFirstNextHop, myTable.findNextHop(myFirstTenant, myAddress));
    EXPECT_EQ(myLastNextHop, myTable.findNextHop(myLastTenant, myAddress));
}

TYPED_TEST(TenantBounds, LookupOutsideCapacityMissesOnFullTable)
{
    const auto myCapacity = TenantCapacity{4};
    const auto myOutsideTenant = TenantId{myCapacity.value()};

    TypeParam myTable{myCapacity};
    addEveryTenantWithDefaultRoute(myTable, myCapacity);

    EXPECT_EQ(std::nullopt, myTable.findNextHop(myOutsideTenant, address(10, 1, 2, 3)));
}

TYPED_TEST(TenantBounds, LookupOnMaxTenantIdMissesOnFullTable)
{
    const auto myCapacity = TenantCapacity{4};
    const auto myMaxTenant = TenantId{std::numeric_limits<std::uint32_t>::max()};

    TypeParam myTable{myCapacity};
    addEveryTenantWithDefaultRoute(myTable, myCapacity);

    EXPECT_EQ(std::nullopt, myTable.findNextHop(myMaxTenant, address(10, 1, 2, 3)));
}

TYPED_TEST(TenantBounds, AddingTenantOutsideCapacityThrows)
{
    const auto myCapacity = TenantCapacity{4};
    const auto myOutsideTenant = TenantId{myCapacity.value()};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};

    TypeParam myTable{myCapacity};

    EXPECT_THROW(myTable.addTenant(myOutsideTenant, routes(Route{myPrefix, myNextHop})),
                 std::invalid_argument);
}

TYPED_TEST(TenantBounds, AddingMaxTenantIdThrows)
{
    const auto myCapacity = TenantCapacity{4};
    const auto myMaxTenant = TenantId{std::numeric_limits<std::uint32_t>::max()};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};

    TypeParam myTable{myCapacity};

    EXPECT_THROW(myTable.addTenant(myMaxTenant, routes(Route{myPrefix, myNextHop})),
                 std::invalid_argument);
}

TYPED_TEST(TenantBounds, UpdateOutsideCapacityThrows)
{
    const auto myCapacity = TenantCapacity{4};
    const auto myOutsideTenant = TenantId{myCapacity.value()};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};

    TypeParam myTable{myCapacity};

    EXPECT_THROW(myTable.update(myOutsideTenant, myPrefix, myNextHop), std::invalid_argument);
}

TYPED_TEST(TenantBounds, RemoveOutsideCapacityThrows)
{
    const auto myCapacity = TenantCapacity{4};
    const auto myOutsideTenant = TenantId{myCapacity.value()};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);

    TypeParam myTable{myCapacity};

    EXPECT_THROW(myTable.remove(myOutsideTenant, myPrefix), std::invalid_argument);
}

TYPED_TEST(TenantBounds, RemovingTenantOutsideCapacityThrows)
{
    const auto myCapacity = TenantCapacity{4};
    const auto myOutsideTenant = TenantId{myCapacity.value()};

    TypeParam myTable{myCapacity};

    EXPECT_THROW(myTable.removeTenant(myOutsideTenant), std::invalid_argument);
}

TYPED_TEST(TenantBounds, FailedAddOutsideCapacityLeavesOthersIntact)
{
    const auto myCapacity = TenantCapacity{4};
    const auto myLastTenant = TenantId{myCapacity.value() - 1};
    const auto myOutsideTenant = TenantId{myCapacity.value()};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myRejectedPrefix = prefix(20, 0, 0, 0, 8);
    const auto myNextHop = NextHop{1};
    const auto myRejectedNextHop = NextHop{2};
    const auto myOtherRejectedNextHop = NextHop{3};

    TypeParam myTable{myCapacity};
    myTable.addTenant(myLastTenant, routes(Route{myPrefix, myNextHop}));
    EXPECT_THROW(
        myTable.addTenant(myOutsideTenant, routes(Route{myPrefix, myRejectedNextHop},
                                                  Route{myRejectedPrefix, myOtherRejectedNextHop})),
        std::invalid_argument);

    EXPECT_EQ(myNextHop, myTable.findNextHop(myLastTenant, address(10, 1, 2, 3)));
    EXPECT_EQ(std::nullopt, myTable.findNextHop(myLastTenant, address(20, 1, 2, 3)));
}

} // namespace mtlpm::tests
