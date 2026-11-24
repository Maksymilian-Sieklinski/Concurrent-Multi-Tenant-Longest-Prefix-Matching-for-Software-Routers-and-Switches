#include "core/shared_types.hpp"
#include "tests/implementations.hpp"
#include "tests/integration/forwarding_table_fixture.hpp"
#include "tests/ipv4_notation.hpp"
#include "tests/routes.hpp"
#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>

namespace mtlpm::tests
{

template <typename Table>
class TenantErrors : public ForwardingTableTest<Table>
{
};
TYPED_TEST_SUITE(TenantErrors, MultiTenantImplementations, ImplementationNames);

TYPED_TEST(TenantErrors, AddingExistingTenantThrows)
{
    auto &myTable = this->theTable;

    const auto myTenant = TenantId{1};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};
    const auto mySecondNextHop = NextHop{2};

    myTable.addTenant(myTenant, routes(Route{myPrefix, myNextHop}));

    EXPECT_THROW(myTable.addTenant(myTenant, routes(Route{myPrefix, mySecondNextHop})),
                 std::invalid_argument);
}

TYPED_TEST(TenantErrors, FailedAddTenantKeepsOriginalRoutes)
{
    auto &myTable = this->theTable;

    const auto myTenant = TenantId{1};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myRejectedPrefix = prefix(20, 0, 0, 0, 8);
    const auto myOriginalNextHop = NextHop{1};
    const auto myRejectedNextHop = NextHop{2};
    const auto myOtherRejectedNextHop = NextHop{3};

    myTable.addTenant(myTenant, routes(Route{myPrefix, myOriginalNextHop}));
    EXPECT_THROW(
        myTable.addTenant(myTenant, routes(Route{myPrefix, myRejectedNextHop},
                                           Route{myRejectedPrefix, myOtherRejectedNextHop})),
        std::invalid_argument);

    EXPECT_EQ(myOriginalNextHop, myTable.findNextHop(myTenant, address(10, 1, 2, 3)));
    EXPECT_EQ(std::nullopt, myTable.findNextHop(myTenant, address(20, 1, 2, 3)));
}

TYPED_TEST(TenantErrors, UpdateOnMissingTenantThrows)
{
    auto &myTable = this->theTable;

    const auto myExistingTenant = TenantId{1};
    const auto myMissingTenant = TenantId{2};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};

    myTable.addTenant(myExistingTenant, routes());

    EXPECT_THROW(myTable.update(myMissingTenant, myPrefix, myNextHop), std::invalid_argument);
}

TYPED_TEST(TenantErrors, RemoveOnMissingTenantThrows)
{
    auto &myTable = this->theTable;

    const auto myExistingTenant = TenantId{1};
    const auto myMissingTenant = TenantId{2};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};

    myTable.addTenant(myExistingTenant, routes(Route{myPrefix, myNextHop}));

    EXPECT_THROW(myTable.remove(myMissingTenant, myPrefix), std::invalid_argument);
}

TYPED_TEST(TenantErrors, RemovingMissingTenantThrows)
{
    auto &myTable = this->theTable;

    const auto myExistingTenant = TenantId{1};
    const auto myMissingTenant = TenantId{2};

    myTable.addTenant(myExistingTenant, routes());

    EXPECT_THROW(myTable.removeTenant(myMissingTenant), std::invalid_argument);
}

TYPED_TEST(TenantErrors, FailedRemoveTenantLeavesOthersIntact)
{
    auto &myTable = this->theTable;

    const auto myExistingTenant = TenantId{1};
    const auto myMissingTenant = TenantId{2};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};
    const auto myAddress = address(10, 1, 2, 3);

    myTable.addTenant(myExistingTenant, routes(Route{myPrefix, myNextHop}));
    EXPECT_THROW(myTable.removeTenant(myMissingTenant), std::invalid_argument);

    EXPECT_EQ(myNextHop, myTable.findNextHop(myExistingTenant, myAddress));
}

TYPED_TEST(TenantErrors, RemovingRemovedTenantThrows)
{
    auto &myTable = this->theTable;

    const auto myTenant = TenantId{1};

    myTable.addTenant(myTenant, routes());
    myTable.removeTenant(myTenant);

    EXPECT_THROW(myTable.removeTenant(myTenant), std::invalid_argument);
}

TYPED_TEST(TenantErrors, UpdateOnRemovedTenantThrows)
{
    auto &myTable = this->theTable;

    const auto myTenant = TenantId{1};
    const auto myPrefix = prefix(10, 1, 0, 0, 16);
    const auto myNextHop = NextHop{1};

    myTable.addTenant(myTenant, routes());
    myTable.removeTenant(myTenant);

    EXPECT_THROW(myTable.update(myTenant, myPrefix, myNextHop), std::invalid_argument);
}

} // namespace mtlpm::tests
