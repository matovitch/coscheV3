#pragma once

#include <unordered_set>
#include <cstddef>

namespace cosche
{

template <class>
class TGraph;

template <class TYPE>
class TNode
{
    template <class>
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
