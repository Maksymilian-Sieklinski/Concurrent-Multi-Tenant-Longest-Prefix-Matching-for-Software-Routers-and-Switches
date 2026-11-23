#include "forwarding_tables/single_tenant/linear_length_search/linear_length_search.hpp"
#include "core/shared_types.hpp"
#include <optional>
#include <ranges>

namespace mtlpm::single_tenant::linear_length_search
{
LinearLengthSearch::LinearLengthSearch(const TenantRoutes &aTenantRoutes)
{
    for (const auto &[myPrefix, myNextHop] : aTenantRoutes.theRoutes)
        update(myPrefix, myNextHop);
}

auto LinearLengthSearch::update(Prefix aPrefix, NextHop aNextHop) -> void
{
    thePrefixToNextHopMap.insert_or_assign(aPrefix, aNextHop);
}

auto LinearLengthSearch::remove(Prefix aPrefix) -> void { thePrefixToNextHopMap.erase(aPrefix); }

auto LinearLengthSearch::findNextHop(Address anAddress) const noexcept -> std::optional<NextHop>
{
    for (const auto myPrefixLength :
         std::views::iota(0, ADDRESS_BITS + 1) | std::views::reverse)
    {
        const auto myPrefix = Prefix(anAddress.value(), myPrefixLength);
        if (thePrefixToNextHopMap.contains(myPrefix))
            return thePrefixToNextHopMap.at(myPrefix);
    }
    return std::nullopt;
}

} // namespace mtlpm::single_tenant::linear_length_search