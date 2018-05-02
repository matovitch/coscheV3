#pragma once

#include "likelyhood.hpp"
#include "singleton.hpp"
#include "factory.hpp"
#include "node.hpp"

#include <unordered_set>
#include <cstddef>

namespace cosche
{

namespace graph
{

template <class Type>
struct TMakeTraits
{
    using Node = TNode<Type>;
    using NodeFactory = TFactory<factory::TMakeTraits<Node>>;
};

} // namespace graph

template <class GraphTraits>
class TGraph
{
    using NodeFactory = typename GraphTraits::NodeFactory;

public:

    using Node = typename GraphTraits::Node;

    TGraph() :
        _nodeFactory{TSingleton<NodeFactory>::instance()}
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
        lhs._dependees.insert(&rhs);
        rhs._dependers.insert(&lhs);

        block(lhs);
    }

    void attachBatch(Node& node, const std::vector<Node*>& dependers)
    {
        attachBatch<std::vector<Node*>>(node, dependers);
    }

    template <std::size_t BATCH_SIZE>
    void attachBatch(Node& node, const std::array<Node*, BATCH_SIZE>& dependers)
    {
        attachBatch<std::array<Node*, BATCH_SIZE>>(node, dependers);
    }

    void detach(Node& lhs,
                Node& rhs)
    {
        lhs._dependees.erase(&rhs);
        rhs._dependers.erase(&lhs);

        if (lhs._dependees.empty())
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
        for (Node* const dependee : node._dependers)
        {
            auto&& _dependees = dependee->_dependees;

            _dependees.erase(&node);

            if (_dependees.empty())
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
            node = *(node->_dependees.begin());
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

    void block(Node& node)
    {
        const auto& fit = _pendings.find(&node);

        if (COSCHE_LIKELY(fit != _pendings.end()))
        {
            _pendings.erase(fit);
            _blockeds.insert(&node);
        }
    }

    template <class NODE_PTR_CONTAINER>
    void attachBatch(Node& node, const NODE_PTR_CONTAINER& dependers)
    {
        if (COSCHE_UNLIKELY(dependers.empty()))
        {
            return;
        }

        for (const auto depender : dependers)
        {
            node     ._dependees.insert(depender);
            depender->_dependers.insert(&node);
        }

        block(node);
    }

    std::unordered_set<Node*> _pendings;
    std::unordered_set<Node*> _blockeds;

    NodeFactory& _nodeFactory;
};

} // namespace cosche