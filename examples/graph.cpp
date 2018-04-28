#include <iostream>
#include <cstdlib>

#include "cosche/graph.hpp"
#include "cosche/utils.hpp"

static constexpr std::size_t GRAPH_NODE_ALLOCATOR_BUFFER_SIZE = 4096;

int main()
{
    std::cout << "Welcome to coscheV3!" << std::endl;

    using Graph = cosche::TGraph<char, GRAPH_NODE_ALLOCATOR_BUFFER_SIZE>;

    Graph graph;

    auto&& nodeA = graph.makeNode('a');
    auto&& nodeB = graph.makeNode('b');
    auto&& nodeC = graph.makeNode('c');
    auto&& nodeD = graph.makeNode('d');

    graph.attach(nodeA,
                 nodeB);

    graph.attach(nodeB,
                 nodeC);

    graph.attach(nodeB,
                 nodeD);

    graph.attach(nodeB,
                 nodeA);

    while (!graph.empty())
    {
        std::cout << graph.top().value << std::endl;
        graph.pop();
    }

    if (graph.isCyclic())
    {
        std::cout << "Warning! We ended on the cycle:" << std::endl;

        for (const auto& node : graph.makeCycle())
        {
            std::cout << node->value << std::endl;
        }
    }

    cosche::cleanUp();

    return EXIT_SUCCESS;
}