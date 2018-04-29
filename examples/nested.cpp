#include <iostream>
#include <cstdlib>

#include "cosche/scheduler.hpp"
#include "cosche/utils.hpp"

static constexpr std::size_t ABSTRACT_TASK_ALLOCATOR_BUFFER_SIZE = 4096;
static constexpr std::size_t CONCRETE_TASK_ALLOCATOR_BUFFER_SIZE = 4096;
static constexpr std::size_t        FUTURE_ALLOCATOR_BUFFER_SIZE = 4096;

using Scheduler = cosche::TScheduler<ABSTRACT_TASK_ALLOCATOR_BUFFER_SIZE, 
                                     CONCRETE_TASK_ALLOCATOR_BUFFER_SIZE, 
                                            FUTURE_ALLOCATOR_BUFFER_SIZE>;

int main()
{
    Scheduler scheduler;

    auto&& rootTask = scheduler.makeTask<void>();

    std::function<void()> rootWork = 
        [&]()
        {
            auto&& leafTask = scheduler.makeTask<void>();

            std::function<void()> leafWork =
                [&]()
                {
                    std::cout << "Inside!" << std::endl;
                };

            cosche::assignWork(leafTask, std::move(leafWork));

            scheduler.attach(rootTask, leafTask);

            std::cout << "Outside!" << std::endl;
        };

    cosche::assignWork(rootTask, std::move(rootWork));

    scheduler.run();

    cosche::cleanUp();

    return EXIT_SUCCESS;
}
