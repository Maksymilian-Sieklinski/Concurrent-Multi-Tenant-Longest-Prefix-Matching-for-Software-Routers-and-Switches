#pragma once
#include "utils/hashable.hpp"
#include <concepts>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace utils
{

template <typename T>
    requires Hashable<T> && std::equality_comparable<T>
class RandomAccessSet
{
  public:
    auto insert(T anElement) -> void
    {
        auto [myIterator, myIsInserted] =
            theElementToIndex.try_emplace(anElement, theElements.size());
        if (!myIsInserted)
            return;
        theElements.emplace_back(anElement);
    }

    auto erase(T anElement) -> void
    {
        auto myIterator = theElementToIndex.find(anElement);
        if (myIterator == theElementToIndex.end())
            return;
        const auto myIndex = myIterator->second;
        theElementToIndex[theElements.back()] = myIndex;
        theElements[myIndex] = std::move(theElements.back());
        theElements.pop_back();
        theElementToIndex.erase(myIterator);
    }

    template <std::uniform_random_bit_generator G>
    [[nodiscard]] auto getRandom(G &anEngine) const -> T
    {
        if (theElements.empty())
            throw std::out_of_range("The set is empty - cannot get a random element");
        const auto myRandomIndex =
            std::uniform_int_distribution<std::size_t>{0, theElements.size() - 1}(anEngine);
        return theElements[myRandomIndex];
    }

    auto contains(T anElement) const -> bool { return theElementToIndex.contains(anElement); }

    [[nodiscard]] auto empty() const noexcept -> bool { return theElements.empty(); }

    auto clear() -> void
    {
        theElements.clear();
        theElementToIndex.clear();
    }

  private:
    std::vector<T> theElements;
    std::unordered_map<T, std::size_t> theElementToIndex;
};

} // namespace utils