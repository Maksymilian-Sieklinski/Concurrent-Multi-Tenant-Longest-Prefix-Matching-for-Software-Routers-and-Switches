#include "core/shared_types.hpp"
#include "tests/implementations.hpp"
#include "tests/integration/forwarding_table_fixture.hpp"
#include "tests/ipv4_notation.hpp"
#include "tests/routes.hpp"
#include <gtest/gtest.h>
#include <optional>

namespace mtlpm::tests
{

namespace
{
constexpr auto TENANT = TenantId{0};
} // namespace

template <typename Table>
class PrefixUpdates : public ForwardingTableTest<Table>
{
};
TYPED_TEST_SUITE(PrefixUpdates, MultiTenantImplementations, ImplementationNames);

TYPED_TEST(PrefixUpdates, UpdatedPrefixMatches)
{
    auto &myTable = this->theTable;

    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(TENANT, routes());
    myTable.update(TENANT, myPrefix, myNextHop);

    EXPECT_EQ(myNextHop, myTable.findNextHop(TENANT, myAddress));
}

TYPED_TEST(PrefixUpdates, UpdateOverwritesNextHop)
{
    auto &myTable = this->theTable;

    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myOldNextHop = NextHop{1};
    const auto myNewNextHop = NextHop{2};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(TENANT, routes(Route{myPrefix, myOldNextHop}));
    myTable.update(TENANT, myPrefix, myNewNextHop);

    EXPECT_EQ(myNewNextHop, myTable.findNextHop(TENANT, myAddress));
}

TYPED_TEST(PrefixUpdates, LongerPrefixStillMatchesInsideShorterPrefixAddedByUpdate)
{
    auto &myTable = this->theTable;

    const auto myShortPrefix = prefix(10, 0, 0, 0, 8);
    const auto myLongPrefix = prefix(10, 1, 0, 0, 16);
    const auto myShortNextHop = NextHop{1};
    const auto myLongNextHop = NextHop{2};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(TENANT, routes(Route{myLongPrefix, myLongNextHop}));
    myTable.update(TENANT, myShortPrefix, myShortNextHop);

    EXPECT_EQ(myLongNextHop, myTable.findNextHop(TENANT, myAddress));
}

TYPED_TEST(PrefixUpdates, ShorterPrefixStillMatchesOutsideLongerPrefixAddedByUpdate)
{
    auto &myTable = this->theTable;

    const auto myShortPrefix = prefix(10, 0, 0, 0, 8);
    const auto myLongPrefix = prefix(10, 1, 0, 0, 16);
    const auto myShortNextHop = NextHop{1};
    const auto myLongNextHop = NextHop{2};
    const auto myAddress = address(10, 0, 1, 1);

    myTable.addTenant(TENANT, routes(Route{myShortPrefix, myShortNextHop}));
    myTable.update(TENANT, myLongPrefix, myLongNextHop);

    EXPECT_EQ(myShortNextHop, myTable.findNextHop(TENANT, myAddress));
}

TYPED_TEST(PrefixUpdates, RemovedPrefixMisses)
{
    auto &myTable = this->theTable;

    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(TENANT, routes(Route{myPrefix, myNextHop}));
    myTable.remove(TENANT, myPrefix);

    EXPECT_EQ(std::nullopt, myTable.findNextHop(TENANT, myAddress));
}

TYPED_TEST(PrefixUpdates, RemovedDefaultRouteMisses)
{
    auto &myTable = this->theTable;

    const auto myDefaultPrefix = prefix(0, 0, 0, 0, 0);
    const auto myNextHop = NextHop{1};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(TENANT, routes(Route{myDefaultPrefix, myNextHop}));
    myTable.remove(TENANT, myDefaultPrefix);

    EXPECT_EQ(std::nullopt, myTable.findNextHop(TENANT, myAddress));
}

TYPED_TEST(PrefixUpdates, RemovedPrefixFallsBackToShorter)
{
    auto &myTable = this->theTable;

    const auto myShortPrefix = prefix(10, 0, 0, 0, 8);
    const auto myLongPrefix = prefix(10, 1, 0, 0, 16);
    const auto myShortNextHop = NextHop{1};
    const auto myLongNextHop = NextHop{2};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(
        TENANT, routes(Route{myShortPrefix, myShortNextHop}, Route{myLongPrefix, myLongNextHop}));
    myTable.remove(TENANT, myLongPrefix);

    EXPECT_EQ(myShortNextHop, myTable.findNextHop(TENANT, myAddress));
}

TYPED_TEST(PrefixUpdates, RemovedShorterPrefixLeavesLongerIntact)
{
    auto &myTable = this->theTable;

    const auto myShortPrefix = prefix(10, 0, 0, 0, 8);
    const auto myLongPrefix = prefix(10, 1, 0, 0, 16);
    const auto myShortNextHop = NextHop{1};
    const auto myLongNextHop = NextHop{2};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(
        TENANT, routes(Route{myShortPrefix, myShortNextHop}, Route{myLongPrefix, myLongNextHop}));
    myTable.remove(TENANT, myShortPrefix);

    EXPECT_EQ(myLongNextHop, myTable.findNextHop(TENANT, myAddress));
}

TYPED_TEST(PrefixUpdates, RemovingAbsentPrefixChangesNothing)
{
    auto &myTable = this->theTable;

    const auto myShortPrefix = prefix(10, 0, 0, 0, 8);
    const auto myLongPrefix = prefix(10, 1, 0, 0, 16);
    const auto myAbsentPrefix = prefix(10, 0b0000'0000, 0, 0, 12);
    const auto myShortNextHop = NextHop{1};
    const auto myLongNextHop = NextHop{2};

    myTable.addTenant(
        TENANT, routes(Route{myShortPrefix, myShortNextHop}, Route{myLongPrefix, myLongNextHop}));
    myTable.remove(TENANT, myAbsentPrefix);

    EXPECT_EQ(myShortNextHop, myTable.findNextHop(TENANT, address(10, 0b0000'0000, 0, 0)));
    EXPECT_EQ(myShortNextHop, myTable.findNextHop(TENANT, address(10, 0b0000'1111, 255, 255)));
    EXPECT_EQ(myLongNextHop, myTable.findNextHop(TENANT, address(10, 0b0000'0001, 2, 3)));
}

TYPED_TEST(PrefixUpdates, ReinsertedPrefixMatchesNewNextHop)
{
    auto &myTable = this->theTable;

    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myOldNextHop = NextHop{1};
    const auto myNewNextHop = NextHop{2};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(TENANT, routes(Route{myPrefix, myOldNextHop}));
    myTable.remove(TENANT, myPrefix);
    myTable.update(TENANT, myPrefix, myNewNextHop);

    EXPECT_EQ(myNewNextHop, myTable.findNextHop(TENANT, myAddress));
}

} // namespace mtlpm::tests
