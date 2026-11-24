#pragma once
#include "forwarding_tables/multi_tenant/multi_tenant_forwarding_table.hpp"
#include "forwarding_tables/multi_tenant/per_tenant_table/per_tenant_table.hpp"
#include "forwarding_tables/single_tenant/binary_trie/binary_trie.hpp"
#include "forwarding_tables/single_tenant/linear_length_search/linear_length_search.hpp"
#include <concepts>
#include <gtest/gtest.h>
#include <string>

namespace mtlpm::tests
{
using Oracle = multi_tenant::per_tenant_table::PerTenantTable<
    single_tenant::linear_length_search::LinearLengthSearch>;
using PerTenantBinaryTrie =
    multi_tenant::per_tenant_table::PerTenantTable<single_tenant::binary_trie::BinaryTrie>;

static_assert(multi_tenant::MultiTenantForwardingTable<Oracle>);
static_assert(multi_tenant::MultiTenantForwardingTable<PerTenantBinaryTrie>);

using MultiTenantImplementations = ::testing::Types<Oracle, PerTenantBinaryTrie>;

struct ImplementationNames
{
    template <typename T>
    // NOLINTNEXTLINE(readability-identifier-naming, modernize-use-string-view)
    static auto GetName([[maybe_unused]] int anIndex) -> std::string
    {
        if constexpr (std::same_as<T, Oracle>)
            return "LinearLengthSearch";
        else if constexpr (std::same_as<T, PerTenantBinaryTrie>)
            return "BinaryTrie";
        else
            static_assert(false, "add a name for this implementation");
    }
};

} // namespace mtlpm::tests