#pragma once

#include "pool_factory.hpp"
#include "likelyhood.hpp"
#include "node.hpp"

#include <unordered_set>
#include <cstddef>

namespace cosche
{

template <class       TYPE,
          std::size_t NODE_ALLOCATOR_BUFFER_SIZE>
class TGraph
{

public:
    
    using Node        = TNode<TYPE>;

    TGraph() :
        _nodeFactory{Singleton<NodeFactory>::instance()}
    {}

    template <class... ARGS>
    Node& makeNode(ARGS&&... args)
    {
        Node& node = _nodeFactory.make(args...);

        _pendings.insert(&node);

        return node;
    }

    void attach(Node& lhs,
                Node& rhs)
    {
        lhs._dependers.insert(&rhs);
        rhs._dependees.insert(&lhs);

        const auto& fit = _pendings.find(&lhs);

        if (COSCHE_LIKELY(fit != _pendings.end()))
        {
            _pendings.erase(fit);
            _blockeds.insert(&lhs);
        }
    }

    void detach(Node& lhs,
                Node& rhs)
    {
        lhs._dependers.erase(&rhs);
        rhs._dependees.erase(&lhs);

        if (lhs._dependers.empty())
        {
            const auto& fit = _blockeds.find(&lhs);

            if (COSCHE_LIKELY(fit != _blockeds.end()))
            {
                _blockeds.erase(fit);
                _pendings.insert(&lhs);
            }
        }
    }

    void detachAll(Node& node)
    {
        for (Node* const dependee : node._dependees)
        {
            auto&& _dependers = dependee->_dependers;

            _dependers.erase(&node);

            if (_dependers.empty())
            {
                _blockeds.erase(dependee);
                _pendings.insert(dependee);
            }
        }
    }


    bool isCyclic()
    {
        return _pendings.empty() && !_blockeds.empty();
    }

    std::vector<Node*> makeCycle()
    {
        if (!isCyclic())
        {
            return {};
        }

        std::vector<Node*> cycle;

        Node* node = *(_blockeds.begin());

        do
        {
            cycle.push_back(node);
            node = *(node->_dependers.begin());
        } 
        while (node != cycle.front());

        return cycle;
    }

    Node& top() const
    {
        return **(_pendings.begin());
    }

    bool empty() const
    {
        return _pendings.empty();
    }

    void pop()
    {
        const auto& topIt = _pendings.begin();

        Node* const top = *topIt;

        detachAll(*top);

        _nodeFactory.recycle(top);

        _pendings.erase(topIt);
    }

private:

    using NodeFactory = pool::TFactory<Node, NODE_ALLOCATOR_BUFFER_SIZE>;

    std::unordered_set<Node*> _pendings;
    std::unordered_set<Node*> _blockeds;

    NodeFactory& _nodeFactory;
};

} // namespace cosche