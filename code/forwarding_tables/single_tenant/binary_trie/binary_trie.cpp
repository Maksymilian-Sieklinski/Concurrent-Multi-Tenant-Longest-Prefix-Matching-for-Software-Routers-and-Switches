#include "forwarding_tables/single_tenant/binary_trie/binary_trie.hpp"
#include "core/address_bits.hpp"
#include "core/shared_types.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <ranges>

namespace mtlpm::single_tenant::binary_trie
{

BinaryTrie::BinaryTrie(const TenantRoutes &aTenantRoutes)
{
    for (const auto &[myPrefix, myNextHop] : aTenantRoutes.theRoutes)
        update(myPrefix, myNextHop);
}

auto BinaryTrie::update(Prefix aPrefix, NextHop aNextHop) -> void
{
    Node *myCurrentNode = &theRoot;
    for (const auto myDepth : std::views::iota(std::uint8_t{0}, aPrefix.theLength()))
    {
        auto &myChild = myCurrentNode->theChildren[addressBit(aPrefix.theBits(), myDepth)];
        if (!myChild)
            myChild = std::make_unique<Node>();
        myCurrentNode = myChild.get();
    }
    myCurrentNode->theNextHop = aNextHop;
}

auto BinaryTrie::remove(Prefix aPrefix) -> void
{
    Node *myCurrentNode = &theRoot;
    for (const auto myDepth : std::views::iota(std::uint8_t{0}, aPrefix.theLength()))
    {
        myCurrentNode = myCurrentNode->theChildren[addressBit(aPrefix.theBits(), myDepth)].get();
        if (!myCurrentNode)
            return;
    }
    myCurrentNode->theNextHop = std::nullopt;
}

auto BinaryTrie::findNextHop(Address anAddress) const noexcept -> std::optional<NextHop>
{
    std::optional<NextHop> myResult = theRoot.theNextHop;
    const Node *myCurrentNode = &theRoot;
    for (const auto myDepth : std::views::iota(std::uint8_t{0}, ADDRESS_BITS))
    {
        myCurrentNode = myCurrentNode->theChildren[addressBit(anAddress.value(), myDepth)].get();
        if (!myCurrentNode)
            return myResult;
        if (myCurrentNode->theNextHop)
            myResult = myCurrentNode->theNextHop;
    }
    return myResult;
}

} // namespace mtlpm::single_tenant::binary_trie
