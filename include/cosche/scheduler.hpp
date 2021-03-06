#pragma once

#include "factory_singleton.hpp"
#include "future_task_pair.hpp"
#include "future.hpp"
#include "graph.hpp"
#include "task.hpp"

#include <optional>
#include <vector>
#include <chrono>

#include <iostream>

namespace cosche
{

namespace scheduler
{

struct Abstract
{
    virtual void pop() = 0;

    virtual ~Abstract();
};

} // namespace scheduler

class Scheduler : public scheduler::Abstract
{
    using TaskGraph = TGraph<task::Abstract*>;

public:

    using TaskNode  = typename TaskGraph::Node;

    Scheduler();

    template<class ReturnType>
    TaskNode& makeTask()
    {
        using Task = TTask<ReturnType>;

        Task& task = TFactorySingleton<Task>::instance().make(*this);

        return _taskGraph.makeNode(&task);
    }

    void attach(TaskNode& lhs,
                TaskNode& rhs);

    void attachBatch(TaskNode& taskNode, const std::vector<TaskNode*>& dependees);

    /* Due to http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1591,
       template deduction doesn't work with std::array. */
    template <std::size_t BATCH_SIZE>
    void attachBatch(TaskNode& taskNode, TaskNode* const (&dependees)[BATCH_SIZE])
    {
        _taskGraph.attachBatch(taskNode, dependees);

        releaseContext(taskNode);
    }


    void detach(TaskNode& lhs,
                TaskNode& rhs);

    void run();

    template <class ReturnType, class Rep, class Period>
    ReturnType attach(TaskNode& taskNode,
                      std::future<ReturnType>&& future,
                      const std::chrono::duration<Rep, Period>& pollingDelay)
    {
        using Future = TFuture<ReturnType, Rep, Period>;

        auto&& theFuture = TFactorySingleton<Future>::instance().make(std::move(future),
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
        using Future = future::TScoped<ReturnType, Rep1, Period1>;

        auto&& theFuture = TFactorySingleton<Future>::instance().make(std::move(future),
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

    void pop() override;

private:

    bool hasFutures();

    void releaseContext(TaskNode& taskNode);

    bool                                             _isRunning;
    TaskGraph                                        _taskGraph;
    std::vector<FutureTaskPair>               _futuresTaskPairs;
};

} // namespace cosche