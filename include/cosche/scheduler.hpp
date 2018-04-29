#pragma once

#include "future_task_pair.hpp"
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

} // namespace scheduler

template <std::size_t ABSTRACT_TASK_ALLOCATOR_BUFFER_SIZE,
          std::size_t CONCRETE_TASK_ALLOCATOR_BUFFER_SIZE,
          std::size_t        FUTURE_ALLOCATOR_BUFFER_SIZE>
class TScheduler : public scheduler::Abstract
{
    using TaskGraph = TGraph<task::Abstract*,
                             ABSTRACT_TASK_ALLOCATOR_BUFFER_SIZE>;

public:

    using TaskNode  = typename TaskGraph::Node;

    TScheduler() : _isRunning{false} {}

    template<class RETURN_TYPE, class... ARGS>
    TaskNode& makeTask()
    {
        using Task        = TTask<RETURN_TYPE, ARGS...>;
        using TaskFactory = pool::TFactory<Task, CONCRETE_TASK_ALLOCATOR_BUFFER_SIZE>;

        Task& task = Singleton<TaskFactory>::instance().make(*this);

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

    template <class RETURN_TYPE, class REP, class PERIOD>
    RETURN_TYPE attach(TaskNode& taskNode, 
                       std::future<RETURN_TYPE>&& future,
                       const std::chrono::duration<REP, PERIOD>& pollingDelay)
    {
        using Future        = TFuture<RETURN_TYPE, REP, PERIOD>;
        using FutureFactory = pool::TFactory<Future, FUTURE_ALLOCATOR_BUFFER_SIZE>;

        auto&& theFuture = Singleton<FutureFactory>::instance().make(std::move(future),
                                                                     pollingDelay);
        _futuresTaskPairs.emplace_back(&theFuture, &taskNode);

        attach(taskNode,
               taskNode);

        return theFuture.value().get();
    }

    template <class RETURN_TYPE, class REP1, class PERIOD1, 
                                 class REP2, class PERIOD2>
    std::optional<RETURN_TYPE> attach(TaskNode& taskNode, 
                                      std::future<RETURN_TYPE>&& future,
                                      const std::chrono::duration<REP1, PERIOD1>& pollingDelay,
                                      const std::chrono::duration<REP2, PERIOD2>& timeoutDuration)
    {
        using Future        = future::TScoped<RETURN_TYPE, REP1, PERIOD1>;
        using FutureFactory = pool::TFactory<Future, FUTURE_ALLOCATOR_BUFFER_SIZE>;

        auto&& theFuture = Singleton<FutureFactory>::instance().make(std::move(future),
                                                                     pollingDelay,
                                                                     timeoutDuration);
        _futuresTaskPairs.emplace_back(&theFuture, &taskNode);

        attach(taskNode,
               taskNode);

        using namespace std::chrono_literals;

        return (theFuture.value().wait_for(0s) == std::future_status::ready) ?
            std::optional<RETURN_TYPE>{theFuture.value().get()}              :
            std::optional<RETURN_TYPE>{};
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