#include "core/shared_types.hpp"
#include "tests/implementations.hpp"
#include "tests/integration/forwarding_table_fixture.hpp"
#include "tests/ipv4_notation.hpp"
#include "tests/routes.hpp"
#include <gtest/gtest.h>
#include <optional>

namespace mtlpm::tests
{

template <typename Table>
class TenantChurn : public ForwardingTableTest<Table>
{
};
TYPED_TEST_SUITE(TenantChurn, MultiTenantImplementations, ImplementationNames);

TYPED_TEST(TenantChurn, LookupOnUnknownTenantMisses)
{
    auto &myTable = this->theTable;

    const auto myUnknownTenant = TenantId{1};
    const auto myAddress = address(10, 1, 2, 3);

    EXPECT_EQ(std::nullopt, myTable.findNextHop(myUnknownTenant, myAddress));
}

TYPED_TEST(TenantChurn, LookupOnRemovedTenantMisses)
{
    auto &myTable = this->theTable;

    const auto myTenant = TenantId{1};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(myTenant, routes(Route{myPrefix, myNextHop}));
    myTable.removeTenant(myTenant);

    EXPECT_EQ(std::nullopt, myTable.findNextHop(myTenant, myAddress));
}

TYPED_TEST(TenantChurn, RemovingTenantLeavesOthersIntact)
{
    auto &myTable = this->theTable;

    const auto myRemovedTenant = TenantId{1};
    const auto myKeptTenant = TenantId{2};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myRemovedNextHop = NextHop{1};
    const auto myKeptNextHop = NextHop{2};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(myRemovedTenant, routes(Route{myPrefix, myRemovedNextHop}));
    myTable.addTenant(myKeptTenant, routes(Route{myPrefix, myKeptNextHop}));
    myTable.removeTenant(myRemovedTenant);

    EXPECT_EQ(myKeptNextHop, myTable.findNextHop(myKeptTenant, myAddress));
}

TYPED_TEST(TenantChurn, TenantAddedAgainAfterRemovalDropsOldRoutes)
{
    auto &myTable = this->theTable;

    const auto myTenant = TenantId{1};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(myTenant, routes(Route{myPrefix, myNextHop}));
    myTable.removeTenant(myTenant);
    myTable.addTenant(myTenant, routes());

    EXPECT_EQ(std::nullopt, myTable.findNextHop(myTenant, myAddress));
}

TYPED_TEST(TenantChurn, TenantAddedAgainAfterRemovalDropsOldUpdates)
{
    auto &myTable = this->theTable;

    const auto myTenant = TenantId{1};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(myTenant, routes());
    myTable.update(myTenant, myPrefix, myNextHop);
    myTable.removeTenant(myTenant);
    myTable.addTenant(myTenant, routes());

    EXPECT_EQ(std::nullopt, myTable.findNextHop(myTenant, myAddress));
}

TYPED_TEST(TenantChurn, InterleavedChurnPreservesLiveTenants)
{
    auto &myTable = this->theTable;

    const auto myFirstTenant = TenantId{1};
    const auto mySecondTenant = TenantId{2};
    const auto myThirdTenant = TenantId{3};
    const auto myFourthTenant = TenantId{4};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myFirstNextHop = NextHop{1};
    const auto mySecondOldNextHop = NextHop{2};
    const auto myThirdNextHop = NextHop{3};
    const auto myFourthNextHop = NextHop{4};
    const auto mySecondNewNextHop = NextHop{5};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(myFirstTenant, routes(Route{myPrefix, myFirstNextHop}));
    myTable.addTenant(mySecondTenant, routes(Route{myPrefix, mySecondOldNextHop}));
    myTable.addTenant(myThirdTenant, routes(Route{myPrefix, myThirdNextHop}));
    myTable.removeTenant(mySecondTenant);
    myTable.addTenant(myFourthTenant, routes(Route{myPrefix, myFourthNextHop}));
    myTable.removeTenant(myFirstTenant);
    myTable.addTenant(mySecondTenant, routes(Route{myPrefix, mySecondNewNextHop}));

    EXPECT_EQ(mySecondNewNextHop, myTable.findNextHop(mySecondTenant, myAddress));
    EXPECT_EQ(myThirdNextHop, myTable.findNextHop(myThirdTenant, myAddress));
    EXPECT_EQ(myFourthNextHop, myTable.findNextHop(myFourthTenant, myAddress));
}

} // namespace mtlpm::tests
