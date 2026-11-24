#include "core/address_bits.hpp"
#include "core/prefix.hpp"
#include "core/shared_types.hpp"
#include "tests/implementations.hpp"
#include "tests/integration/forwarding_table_fixture.hpp"
#include "tests/ipv4_notation.hpp"
#include "tests/routes.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <optional>
#include <ranges>
#include <vector>

namespace mtlpm::tests
{

namespace
{
constexpr auto TENANT = TenantId{0};
} // namespace

template <typename Table>
class LongestPrefixMatch : public ForwardingTableTest<Table>
{
};
TYPED_TEST_SUITE(LongestPrefixMatch, MultiTenantImplementations, ImplementationNames);

TYPED_TEST(LongestPrefixMatch, LongerPrefixWins)
{
    auto &myTable = this->theTable;

    const auto myShortPrefix = prefix(10, 0, 0, 0, 8);
    const auto myLongPrefix = prefix(10, 1, 0, 0, 16);
    const auto myShortNextHop = NextHop{1};
    const auto myLongNextHop = NextHop{2};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(
        TENANT, routes(Route{myLongPrefix, myLongNextHop}, Route{myShortPrefix, myShortNextHop}));

    EXPECT_EQ(myLongNextHop, myTable.findNextHop(TENANT, myAddress));
}

TYPED_TEST(LongestPrefixMatch, AddressOnEmptyTenantMisses)
{
    auto &myTable = this->theTable;

    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(TENANT, routes());

    EXPECT_EQ(std::nullopt, myTable.findNextHop(TENANT, myAddress));
}

TYPED_TEST(LongestPrefixMatch, AddressOutsideEveryPrefixMisses)
{
    auto &myTable = this->theTable;

    const auto myFirstPrefix = prefix(10, 0, 0, 0, 8);
    const auto mySecondPrefix = prefix(192, 168, 0, 0, 16);
    const auto myFirstNextHop = NextHop{1};
    const auto mySecondNextHop = NextHop{2};
    const auto myAddress = address(192, 169, 0, 1);

    myTable.addTenant(TENANT, routes(Route{myFirstPrefix, myFirstNextHop},
                                     Route{mySecondPrefix, mySecondNextHop}));

    EXPECT_EQ(std::nullopt, myTable.findNextHop(TENANT, myAddress));
}

TYPED_TEST(LongestPrefixMatch, DefaultRouteCoversEveryAddress)
{
    auto &myTable = this->theTable;

    const auto myDefaultPrefix = prefix(0, 0, 0, 0, 0);
    const auto myNextHop = NextHop{1};

    myTable.addTenant(TENANT, routes(Route{myDefaultPrefix, myNextHop}));

    EXPECT_EQ(myNextHop, myTable.findNextHop(TENANT, address(0, 0, 0, 0)));
    EXPECT_EQ(myNextHop, myTable.findNextHop(TENANT, address(127, 255, 255, 255)));
    EXPECT_EQ(myNextHop, myTable.findNextHop(TENANT, address(128, 0, 0, 0)));
    EXPECT_EQ(myNextHop, myTable.findNextHop(TENANT, address(10, 1, 2, 3)));
    EXPECT_EQ(myNextHop, myTable.findNextHop(TENANT, address(255, 255, 255, 255)));
}

TYPED_TEST(LongestPrefixMatch, HostRouteMatchesOnlyItsAddress)
{
    auto &myTable = this->theTable;

    const auto myHostPrefix = prefix(10, 1, 2, 3, 32);
    const auto myNextHop = NextHop{1};

    myTable.addTenant(TENANT, routes(Route{myHostPrefix, myNextHop}));

    EXPECT_EQ(myNextHop, myTable.findNextHop(TENANT, address(10, 1, 2, 3)));
    EXPECT_EQ(std::nullopt, myTable.findNextHop(TENANT, address(10, 1, 2, 2)));
    EXPECT_EQ(std::nullopt, myTable.findNextHop(TENANT, address(10, 1, 2, 4)));
    EXPECT_EQ(std::nullopt, myTable.findNextHop(TENANT, address(138, 1, 2, 3)));
}

TYPED_TEST(LongestPrefixMatch, PrefixCoversExactlyItsRange)
{
    auto &myTable = this->theTable;

    const auto myPrefix = prefix(10, 1, 0b0001'0000, 0, 20);
    const auto myNextHop = NextHop{1};

    myTable.addTenant(TENANT, routes(Route{myPrefix, myNextHop}));

    EXPECT_EQ(std::nullopt, myTable.findNextHop(TENANT, address(10, 1, 0b0000'1111, 255)));
    EXPECT_EQ(myNextHop, myTable.findNextHop(TENANT, address(10, 1, 0b0001'0000, 0)));
    EXPECT_EQ(myNextHop, myTable.findNextHop(TENANT, address(10, 1, 0b0001'1111, 255)));
    EXPECT_EQ(std::nullopt, myTable.findNextHop(TENANT, address(10, 1, 0b0010'0000, 0)));
}

TYPED_TEST(LongestPrefixMatch, PrefixesDifferingOnlyInLastBitMatchSeparateAddresses)
{
    auto &myTable = this->theTable;

    const auto myEvenPrefix = prefix(10, 1, 0b00000010, 0, 24);
    const auto myOddPrefix = prefix(10, 1, 0b00000011, 0, 24);
    const auto myEvenNextHop = NextHop{1};
    const auto myOddNextHop = NextHop{2};

    myTable.addTenant(TENANT,
                      routes(Route{myEvenPrefix, myEvenNextHop}, Route{myOddPrefix, myOddNextHop}));

    EXPECT_EQ(myEvenNextHop, myTable.findNextHop(TENANT, address(10, 1, 0b00000010, 5)));
    EXPECT_EQ(myOddNextHop, myTable.findNextHop(TENANT, address(10, 1, 0b00000011, 5)));
}

TYPED_TEST(LongestPrefixMatch, SameBitsDifferentLengthPrefixesAreDistinct)
{
    auto &myTable = this->theTable;

    const auto myShortPrefix = prefix(10, 0, 0, 0, 8);
    const auto myLongPrefix = prefix(10, 0, 0, 0, 16);
    const auto myShortNextHop = NextHop{1};
    const auto myLongNextHop = NextHop{2};

    myTable.addTenant(
        TENANT, routes(Route{myShortPrefix, myShortNextHop}, Route{myLongPrefix, myLongNextHop}));

    EXPECT_EQ(myLongNextHop, myTable.findNextHop(TENANT, address(10, 0, 5, 5)));
    EXPECT_EQ(myShortNextHop, myTable.findNextHop(TENANT, address(10, 1, 0, 0)));
}

TYPED_TEST(LongestPrefixMatch, EveryPrefixLengthFromZeroToThirtyTwoMatches)
{
    auto &myTable = this->theTable;

    const auto myLengths = std::views::iota(std::uint8_t{0}, std::uint8_t{ADDRESS_BITS + 1});
    const auto myRoutes =
        myLengths |
        std::views::transform(
            [](std::uint8_t aLength) -> Route
            { return Route{Prefix{prefixMask(aLength), aLength}, NextHop{aLength}}; }) |
        std::ranges::to<std::vector>();

    myTable.addTenant(TENANT, TenantRoutes{myRoutes});

    for (const auto myLength : myLengths)
        EXPECT_EQ(NextHop{myLength}, myTable.findNextHop(TENANT, Address{prefixMask(myLength)}));
}

TYPED_TEST(LongestPrefixMatch, MinAndMaxNextHopsAreReturned)
{
    auto &myTable = this->theTable;

    const auto myMinPrefix = prefix(10, 0, 0, 0, 8);
    const auto myMaxPrefix = prefix(20, 0, 0, 0, 8);
    const auto myMinNextHop = NextHop{std::numeric_limits<std::uint16_t>::min()};
    const auto myMaxNextHop = NextHop{std::numeric_limits<std::uint16_t>::max()};

    myTable.addTenant(TENANT,
                      routes(Route{myMinPrefix, myMinNextHop}, Route{myMaxPrefix, myMaxNextHop}));

    EXPECT_EQ(myMinNextHop, myTable.findNextHop(TENANT, address(10, 1, 2, 3)));
    EXPECT_EQ(myMaxNextHop, myTable.findNextHop(TENANT, address(20, 1, 2, 3)));
}

} // namespace mtlpm::tests
