#include "core/shared_types.hpp"
#include "tests/implementations.hpp"
#include "tests/integration/forwarding_table_fixture.hpp"
#include "tests/ipv4_notation.hpp"
#include "tests/routes.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <optional>

namespace mtlpm::tests
{

template <typename Table>
class TenantIsolation : public ForwardingTableTest<Table>
{
};
TYPED_TEST_SUITE(TenantIsolation, MultiTenantImplementations, ImplementationNames);

TYPED_TEST(TenantIsolation, SamePrefixDifferentNextHopResolvesPerTenant)
{
    auto &myTable = this->theTable;

    const auto myTenant = TenantId{1};
    const auto myOtherTenant = TenantId{2};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};
    const auto myOtherNextHop = NextHop{2};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(myTenant, routes(Route{myPrefix, myNextHop}));
    myTable.addTenant(myOtherTenant, routes(Route{myPrefix, myOtherNextHop}));

    EXPECT_EQ(myNextHop, myTable.findNextHop(myTenant, myAddress));
    EXPECT_EQ(myOtherNextHop, myTable.findNextHop(myOtherTenant, myAddress));
}

TYPED_TEST(TenantIsolation, MinAndMaxTenantIdsResolvePerTenant)
{
    auto &myTable = this->theTable;

    const auto myMinTenant = TenantId{std::numeric_limits<std::uint32_t>::min()};
    const auto myMaxTenant = TenantId{std::numeric_limits<std::uint32_t>::max()};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myMinTenantNextHop = NextHop{1};
    const auto myMaxTenantNextHop = NextHop{2};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(myMinTenant, routes(Route{myPrefix, myMinTenantNextHop}));
    myTable.addTenant(myMaxTenant, routes(Route{myPrefix, myMaxTenantNextHop}));

    EXPECT_EQ(myMinTenantNextHop, myTable.findNextHop(myMinTenant, myAddress));
    EXPECT_EQ(myMaxTenantNextHop, myTable.findNextHop(myMaxTenant, myAddress));
}

TYPED_TEST(TenantIsolation, PrefixDoesNotLeakIntoEmptyTenant)
{
    auto &myTable = this->theTable;

    const auto myTenant = TenantId{1};
    const auto myEmptyTenant = TenantId{2};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(myTenant, routes(Route{myPrefix, myNextHop}));
    myTable.addTenant(myEmptyTenant, routes());

    EXPECT_EQ(std::nullopt, myTable.findNextHop(myEmptyTenant, myAddress));
}

TYPED_TEST(TenantIsolation, DisjointPrefixDoesNotLeakAcrossTenants)
{
    auto &myTable = this->theTable;

    const auto myTenant = TenantId{1};
    const auto myOtherTenant = TenantId{2};
    const auto myPrefix = prefix(10, 0, 0, 0, 8);
    const auto myOtherPrefix = prefix(20, 0, 0, 0, 8);
    const auto myNextHop = NextHop{1};
    const auto myOtherNextHop = NextHop{2};

    myTable.addTenant(myTenant, routes(Route{myPrefix, myNextHop}));
    myTable.addTenant(myOtherTenant, routes(Route{myOtherPrefix, myOtherNextHop}));

    EXPECT_EQ(std::nullopt, myTable.findNextHop(myTenant, address(20, 1, 2, 3)));
    EXPECT_EQ(std::nullopt, myTable.findNextHop(myOtherTenant, address(10, 1, 2, 3)));
}

TYPED_TEST(TenantIsolation, LongerPrefixDoesNotLeakAcrossTenants)
{
    auto &myTable = this->theTable;

    const auto myShortTenant = TenantId{1};
    const auto myLongTenant = TenantId{2};
    const auto myShortPrefix = prefix(10, 0, 0, 0, 8);
    const auto myLongPrefix = prefix(10, 1, 0, 0, 16);
    const auto myShortNextHop = NextHop{1};
    const auto myLongNextHop = NextHop{2};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(myShortTenant, routes(Route{myShortPrefix, myShortNextHop}));
    myTable.addTenant(myLongTenant, routes(Route{myLongPrefix, myLongNextHop}));

    EXPECT_EQ(myShortNextHop, myTable.findNextHop(myShortTenant, myAddress));
}

TYPED_TEST(TenantIsolation, UpdateAffectsOnlyItsTenant)
{
    auto &myTable = this->theTable;

    const auto myTenant = TenantId{1};
    const auto myOtherTenant = TenantId{2};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};
    const auto myOtherNextHop = NextHop{2};
    const auto myUpdatedNextHop = NextHop{3};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(myTenant, routes(Route{myPrefix, myNextHop}));
    myTable.addTenant(myOtherTenant, routes(Route{myPrefix, myOtherNextHop}));
    myTable.update(myTenant, myPrefix, myUpdatedNextHop);

    EXPECT_EQ(myUpdatedNextHop, myTable.findNextHop(myTenant, myAddress));
    EXPECT_EQ(myOtherNextHop, myTable.findNextHop(myOtherTenant, myAddress));
}

TYPED_TEST(TenantIsolation, RemoveAffectsOnlyItsTenant)
{
    auto &myTable = this->theTable;

    const auto myTenant = TenantId{1};
    const auto myOtherTenant = TenantId{2};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};
    const auto myOtherNextHop = NextHop{2};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(myTenant, routes(Route{myPrefix, myNextHop}));
    myTable.addTenant(myOtherTenant, routes(Route{myPrefix, myOtherNextHop}));
    myTable.remove(myTenant, myPrefix);

    EXPECT_EQ(std::nullopt, myTable.findNextHop(myTenant, myAddress));
    EXPECT_EQ(myOtherNextHop, myTable.findNextHop(myOtherTenant, myAddress));
}

TYPED_TEST(TenantIsolation, UpdatedPrefixDoesNotLeakAcrossTenants)
{
    auto &myTable = this->theTable;

    const auto myTenant = TenantId{1};
    const auto myOtherTenant = TenantId{2};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(myTenant, routes());
    myTable.addTenant(myOtherTenant, routes());
    myTable.update(myTenant, myPrefix, myNextHop);

    EXPECT_EQ(std::nullopt, myTable.findNextHop(myOtherTenant, myAddress));
}

} // namespace mtlpm::tests