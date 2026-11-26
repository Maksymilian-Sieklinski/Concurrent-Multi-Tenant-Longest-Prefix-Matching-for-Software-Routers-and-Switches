#pragma once

namespace utils
{
template <typename... Fs>
struct Overload : Fs...
{
    using Fs::operator()...;
};
} // namespace utils