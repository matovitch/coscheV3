#include "cosche/scheduler.hpp"

namespace cosche
{

namespace scheduler
{

Abstract::~Abstract() {}

} // namespace scheduler

Scheduler::Scheduler() : _isRunning{false} {}

void Scheduler::attach(TaskNode& lhs,
                       TaskNode& rhs)
{
    _taskGraph.attach(lhs,
                      rhs);

    releaseContext(lhs);
}

void Scheduler::attachBatch(TaskNode& taskNode,
                            const std::vector<TaskNode*>& dependees)
{
    _taskGraph.attachBatch(taskNode, dependees);

    releaseContext(taskNode);
}

void Scheduler::detach(TaskNode& lhs,
                       TaskNode& rhs)
{
    _taskGraph.detach(lhs,
                      rhs);
}

void Scheduler::run()
{
    _isRunning = true;

    while (!_taskGraph.empty() || hasFutures())
    {
        auto&& topTask = *(_taskGraph.top().value);

        topTask._context = std::move(std::get<0>(topTask._context(&topTask)));
    }

    _isRunning = false;
}

void Scheduler::pop()
{
    _taskGraph.pop();
}

bool Scheduler::hasFutures()
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

void Scheduler::releaseContext(TaskNode& taskNode)
{
    if (COSCHE_LIKELY(_isRunning))
    {
        auto&& task = *(taskNode.value);

        task._context = std::move(std::get<0>(task._context(&task)));
    }
}

} // namespace cosche
