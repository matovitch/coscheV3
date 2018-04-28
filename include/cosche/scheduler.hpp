#pragma once

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

    using TaskNode  = typename TaskGraph::Node;
public:

    TScheduler() : isRunning{false} {}

    template<class RETURN_TYPE, class... ARGS>
    TaskNode& makeTask()
    {
        using Task                 = TTask<RETURN_TYPE, ARGS...>;
        using TaskFactory          = pool::TFactory<Task, CONCRETE_TASK_ALLOCATOR_BUFFER_SIZE>;
        using TaskFactorySingleton = Singleton<TaskFactory>;

        Task& task = TaskFactorySingleton::instance().make(*this);

        return _taskGraph.makeNode(&task);
    }

    void attach(TaskNode& lhs,
                TaskNode& rhs)
    {
        _taskGraph.attach(lhs,
                          rhs);

        if (COSCHE_LIKELY(isRunning))
        {
            auto&& lhsTask = *(lhs.value);

            lhsTask._context = std::move(std::get<0>(lhsTask._context(&lhsTask)));
        }
    }

    void detach(TaskNode& lhs,
                TaskNode& rhs)
    {
        _taskGraph.detach(lhs,
                          rhs);
    }

    void run()
    {
        isRunning = true;

        while (!_taskGraph.empty() || checkFutures())
        {
            auto&& topTask = *(_taskGraph.top().value);

            topTask._context = std::move(std::get<0>(topTask._context(&topTask)));
        }

        isRunning = false;
    }

    template <class RETURN_TYPE, class REP, class PERIOD>
    RETURN_TYPE attach(TaskNode& taskNode, 
                       std::future<RETURN_TYPE>&& future,
                       const std::chrono::duration<REP, PERIOD>& pollingDelay)
    {
        using Future                 = TFuture<RETURN_TYPE, REP, PERIOD>;
        using FutureFactory          = pool::TFactory<Future, FUTURE_ALLOCATOR_BUFFER_SIZE>;
        using FutureFactorySingleton = Singleton<FutureFactory>;

        auto&& theFuture = FutureFactorySingleton::instance().make(std::move(future),
                                                                   pollingDelay);

        _futuresToTasks.emplace(&theFuture, &taskNode);

        attach(taskNode,
               taskNode);

        _futuresToTasks.erase(&theFuture);

        return theFuture.value().get();
    }

    template <class RETURN_TYPE, class REP1, class PERIOD1, 
                                 class REP2, class PERIOD2>
    std::optional<RETURN_TYPE> attach(TaskNode& taskNode, 
                                      std::future<RETURN_TYPE>&& future,
                                      const std::chrono::duration<REP1, PERIOD1>& pollingDelay,
                                      const std::chrono::duration<REP2, PERIOD2>& timeoutDuration)
    {
        using Future                 = future::TScoped<RETURN_TYPE, REP1, PERIOD1>;
        using FutureFactory          = pool::TFactory<Future, FUTURE_ALLOCATOR_BUFFER_SIZE>;
        using FutureFactorySingleton = Singleton<FutureFactory>;

        auto&& theFuture = FutureFactorySingleton::instance().make(std::move(future),
                                                                   pollingDelay,
                                                                   timeoutDuration);

        _futuresToTasks.emplace(&theFuture, &taskNode);

        attach(taskNode,
               taskNode);

        using namespace std::chrono_literals;

        _futuresToTasks.erase(&theFuture);

        return (theFuture.value().wait_for(0s) == std::future_status::ready) ?
            std::optional<RETURN_TYPE>{theFuture.value().get()}              :
            std::optional<RETURN_TYPE>{};
    }

    void pop() override
    {
        _taskGraph.pop();
    }

private:

    bool checkFutures()
    {
        bool returnValue = !_futuresToTasks.empty();

        for (const auto& [futurePtr, taskNodePtr] : _futuresToTasks)
        {
            auto&& task = *taskNodePtr;

            if (futurePtr->ready())
            {
                detach(task, 
                       task);
            }
        }

        return returnValue;
    }

    std::unordered_map<future::Abstract*, TaskNode*> _futuresToTasks;

    TaskGraph _taskGraph;

public:

    bool isRunning;
};

} // namespace cosche