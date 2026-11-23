#pragma once
#include "core/shared_types.hpp"
#include "forwarding_tables/single_tenant/binary_trie/node.hpp"
#include "forwarding_tables/single_tenant/single_tenant_forwarding_table.hpp"
#include <optional>

namespace mtlpm::single_tenant::binary_trie
{

class BinaryTrie
{
  public:
    explicit BinaryTrie(const TenantRoutes &aTenantRoutes);
    auto update(Prefix aPrefix, NextHop aNextHop) -> void;
    auto remove(Prefix aPrefix) -> void;
    [[nodiscard]] auto findNextHop(Address anAddress) const noexcept -> std::optional<NextHop>;

  private:
    Node theRoot;
};

static_assert(SingleTenantForwardingTable<BinaryTrie>);

} // namespace mtlpm::single_tenant::binary_trie