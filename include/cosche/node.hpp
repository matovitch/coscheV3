#pragma once

#include <unordered_set>
#include <cstddef>

namespace cosche
{

template <class, std::size_t>
class TGraph;

template <class TYPE>
class TNode
{
    template <class, std::size_t>
    friend class TGraph;

private:

    std::unordered_set<TNode<TYPE>*> _dependers;
    std::unordered_set<TNode<TYPE>*> _dependees;

public:

    template <class... ARGS>
    TNode(ARGS&&... args) : value{args...} {}

    TYPE value;
};

} // namespace cosche
