#pragma once
#include "core/shared_types.hpp"
#include <array>
#include <memory>
#include <optional>

namespace mtlpm::single_tenant::binary_trie
{

struct Node
{
    std::array<std::unique_ptr<Node>, 2> theChildren;
    std::optional<NextHop> theNextHop;
};

} // namespace mtlpm::single_tenant::binary_trie