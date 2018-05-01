#pragma once

#include "future_task_pair.hpp"
#include "factory.hpp"
#include "future.hpp"
#include "graph.hpp"
#include "task.hpp"

#include <optional>
#include <vector>
#include <chrono>

namespace cosche
{

namespace scheduler
{

struct Abstract
{
    virtual void pop() = 0;
};

template <std::size_t ABSTRACT_TASK_ALLOCATOR_BUFFER_SIZE = 1,
          std::size_t CONCRETE_TASK_ALLOCATOR_BUFFER_SIZE = 1,
          std::size_t        FUTURE_ALLOCATOR_BUFFER_SIZE = 1>
struct TMakeTraits
{
    using TaskGraphTraits = graph::TMakeTraits<task::Abstract*, ABSTRACT_TASK_ALLOCATOR_BUFFER_SIZE>;

    template <class Task>
    using TMakeTaskFactoryTraits = factory::TMakeTraits<Task, CONCRETE_TASK_ALLOCATOR_BUFFER_SIZE>;

    template <class Future>
    using TMakeFutureFactoryTraits = factory::TMakeTraits<Future, FUTURE_ALLOCATOR_BUFFER_SIZE>;
};

} // namespace scheduler

template <class SchedulerTraits = scheduler::TMakeTraits<>>
class TScheduler : public scheduler::Abstract
{
    using TaskGraphTraits = typename SchedulerTraits::TaskGraphTraits;
    using TaskGraph       = TGraph<TaskGraphTraits>;

    template <class Task>
    using TMakeTaskFactoryTraits = typename SchedulerTraits::template TMakeTaskFactoryTraits<Task>;

    template <class Future>
    using TMakeFutureFactoryTraits = typename SchedulerTraits::template TMakeFutureFactoryTraits<Future>;

    template <class Future>
    using TMakeFutureFactory = TFactory<TMakeFutureFactoryTraits<Future>>;

public:

    using TaskNode  = typename TaskGraph::Node;

    TScheduler() : _isRunning{false} {}

    template<class ReturnType>
    TaskNode& makeTask()
    {
        using Task              = TTask<ReturnType>;
        using TaskFactoryTraits = TMakeTaskFactoryTraits<Task>;
        using TaskFactory       = TFactory<TaskFactoryTraits>;

        Task& task = TSingleton<TaskFactory>::instance().make(*this);

        return _taskGraph.makeNode(&task);
    }

    void attach(TaskNode& lhs,
                TaskNode& rhs)
    {
        _taskGraph.attach(lhs,
                          rhs);

        releaseContext(lhs);
    }

    void attachBatch(TaskNode& taskNode, const std::vector<TaskNode*>& dependers)
    {
        _taskGraph.attachBatch(taskNode, dependers);

        releaseContext(taskNode);
    }

    template <std::size_t BATCH_SIZE>
    void attachBatch(TaskNode& taskNode, const std::array<TaskNode*, BATCH_SIZE>& dependers)
    {
        _taskGraph.template attachBatch<BATCH_SIZE>(taskNode, dependers);

        releaseContext(taskNode);
    }

    void detach(TaskNode& lhs,
                TaskNode& rhs)
    {
        _taskGraph.detach(lhs,
                          rhs);
    }

    void run()
    {
        _isRunning = true;

        while (!_taskGraph.empty() || hasFutures())
        {
            auto&& topTask = *(_taskGraph.top().value);

            topTask._context = std::move(std::get<0>(topTask._context(&topTask)));
        }

        _isRunning = false;
    }

    template <class ReturnType, class Rep, class Period>
    ReturnType attach(TaskNode& taskNode,
                      std::future<ReturnType>&& future,
                      const std::chrono::duration<Rep, Period>& pollingDelay)
    {
        using Future        = TFuture<ReturnType, Rep, Period>;
        using FutureFactory = TMakeFutureFactory<Future>;

        auto&& theFuture = TSingleton<FutureFactory>::instance().make(std::move(future),
                                                                      pollingDelay);
        _futuresTaskPairs.emplace_back(&theFuture, &taskNode);

        attach(taskNode,
               taskNode);

        return theFuture.value().get();
    }

    template <class ReturnType, class Rep1, class Period1,
                                class Rep2, class Period2>
    std::optional<ReturnType> attach(TaskNode& taskNode,
                                     std::future<ReturnType>&& future,
                                     const std::chrono::duration<Rep1, Period1>& pollingDelay,
                                     const std::chrono::duration<Rep2, Period2>& timeoutDuration)
    {
        using Future        = future::TScoped<ReturnType, Rep1, Period1>;
        using FutureFactory = TMakeFutureFactory<Future>;

        auto&& theFuture = TSingleton<FutureFactory>::instance().make(std::move(future),
                                                                      pollingDelay,
                                                                      timeoutDuration);
        _futuresTaskPairs.emplace_back(&theFuture, &taskNode);

        attach(taskNode,
               taskNode);

        using namespace std::chrono_literals;

        return (theFuture.value().wait_for(0s) == std::future_status::ready) ?
            std::optional<ReturnType>{theFuture.value().get()}              :
            std::optional<ReturnType>{};
    }

    void pop() override
    {
        _taskGraph.pop();
    }

private:

    bool hasFutures()
    {
        bool returnValue = !_futuresTaskPairs.empty();

        for (auto&& futureTaskPair : _futuresTaskPairs)
        {
            const auto& [futurePtr, taskNodePtr] = futureTaskPair;

            auto&& taskNode = *taskNodePtr;

            if (futurePtr->ready())
            {
                detach(taskNode,
                       taskNode);

                futureTaskPair = _futuresTaskPairs.back();

                _futuresTaskPairs.pop_back();
            }
        }

        return returnValue;
    }

    void releaseContext(TaskNode& taskNode)
    {
        if (COSCHE_LIKELY(_isRunning))
        {
            auto&& task = *(taskNode.value);

            task._context = std::move(std::get<0>(task._context(&task)));
        }
    }

    bool                                             _isRunning;
    TaskGraph                                        _taskGraph;
    std::vector<FutureTaskPair>               _futuresTaskPairs;
};

} // namespace cosche