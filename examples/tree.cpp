#include <iostream>
#include <cstdlib>

#include "cosche/scheduler.hpp"
#include "cosche/utils.hpp"

static constexpr std::size_t ABSTRACT_TASK_ALLOCATOR_BUFFER_SIZE = 4096;
static constexpr std::size_t CONCRETE_TASK_ALLOCATOR_BUFFER_SIZE = 4096;
static constexpr std::size_t        FUTURE_ALLOCATOR_BUFFER_SIZE = 4096;

static constexpr unsigned TREE_DEPTH = 5;

using Scheduler = cosche::TScheduler<ABSTRACT_TASK_ALLOCATOR_BUFFER_SIZE, 
                                     CONCRETE_TASK_ALLOCATOR_BUFFER_SIZE, 
                                            FUTURE_ALLOCATOR_BUFFER_SIZE>;

using TaskNode = typename Scheduler::TaskNode;

std::function<void()> makeRecursiveWork(const unsigned stackDepth, Scheduler& scheduler, TaskNode* const task)
{
    if (stackDepth == TREE_DEPTH)
    {
        return 
            [&]()
            {
                std::cout << "Here is a leaf!" << std::endl;
            };
    }

    return
        [stackDepth, &scheduler, task]()
        {
            auto&& left  = scheduler.makeTask<void>();
            auto&& right = scheduler.makeTask<void>();

            cosche::assignWork(left  , makeRecursiveWork(stackDepth + 1, scheduler, &left  ));
            cosche::assignWork(right , makeRecursiveWork(stackDepth + 1, scheduler, &right ));

            scheduler.attachBatch<2>(*task, {&left, &right});

            std::cout << "Here is a node!" << std::endl;
        };
}

int main()
{
    Scheduler scheduler;

    auto&& rootTask = scheduler.makeTask<void>();

    auto&& rootWork = makeRecursiveWork(0, scheduler, &rootTask);

    cosche::assignWork(rootTask, std::move(rootWork));

    scheduler.run();

    cosche::cleanUp();

    return EXIT_SUCCESS;
}
