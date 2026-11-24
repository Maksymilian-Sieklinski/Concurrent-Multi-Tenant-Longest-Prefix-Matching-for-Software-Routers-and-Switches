#include "core/shared_types.hpp"
#include "tests/implementations.hpp"
#include "tests/integration/forwarding_table_fixture.hpp"
#include "tests/ipv4_notation.hpp"
#include <gtest/gtest.h>
namespace mtlpm::tests
{

namespace
{
constexpr auto TENANT = TenantId{0};
constexpr auto NEXT_HOP = NextHop{0};
} // namespace

template <typename Table>
class LongestPrefixMatch : public ForwardingTableTest<Table>
{
};
TYPED_TEST_SUITE(LongestPrefixMatch, MultiTenantImplementations, ImplementationNames);

TYPED_TEST(LongestPrefixMatch, UpdatedPrefixMatches)
{
    auto &myTable = this->theTable;

    const auto myPrefix = prefix(1, 1, 0, 0, 16);
    const auto myAddress = address(1, 1, 2, 4);

    myTable.addTenant(TENANT, {});
    myTable.update(TENANT, myPrefix, NEXT_HOP);

    const auto myPresentNextHop = myTable.findNextHop(TENANT, myAddress);
    EXPECT_EQ(NEXT_HOP, myPresentNextHop);
}

TYPED_TEST(LongestPrefixMatch, AddressOnEmptyTenantMisses)
{
    auto &myTable = this->theTable;

    const auto myAddress = address(1, 1, 2, 4);

    myTable.addTenant(TENANT, {});
    const auto myMissingNextHop = myTable.findNextHop(TENANT, myAddress);
    EXPECT_EQ(std::nullopt, myMissingNextHop);
}

TYPED_TEST(LongestPrefixMatch, LongestPrefixMatches)
{
    auto &myTable = this->theTable;

    const auto myAddress = address(1, 1, 2, 4);

    const auto myShortPrefix = prefix(1, 0, 0, 0, 8);
    const auto myShortNextHop = NextHop{0};

    myTable.addTenant(TENANT, {});
    myTable.update(TENANT, myShortPrefix, myShortNextHop);
    EXPECT_EQ(myShortNextHop, myTable.findNextHop(TENANT, myAddress));

    const auto myLongPrefix = prefix(1, 1, 0, 0, 16);
    const auto myLongNextHop = NextHop{1};

    myTable.update(TENANT, myLongPrefix, myLongNextHop);
    EXPECT_EQ(myLongNextHop, myTable.findNextHop(TENANT, myAddress));
}

} // namespace mtlpm::tests