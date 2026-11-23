#pragma once
#include "core/shared_types.hpp"
#include "forwarding_tables/single_tenant/single_tenant_forwarding_table.hpp"
#include <optional>
#include <unordered_map>

namespace mtlpm::single_tenant::linear_length_search
{

class LinearLengthSearch
{
  public:
    explicit LinearLengthSearch(const TenantRoutes &);
    auto update(Prefix aPrefix, NextHop aNextHop) -> void;
    auto remove(Prefix aPrefix) -> void;
    [[nodiscard]] auto findNextHop(Address anAddress) const noexcept -> std::optional<NextHop>;

  private:
    std::unordered_map<Prefix, NextHop> thePrefixToNextHopMap;
};

static_assert(SingleTenantForwardingTable<LinearLengthSearch>);

} // namespace mtlpm::single_tenant::linear_length_search