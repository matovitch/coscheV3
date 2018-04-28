#include "cosche/task.hpp"

#include "cosche/scheduler.hpp"

namespace cosche
{

namespace task
{

Abstract::Context Abstract::entryPoint(Abstract::Context&& context, 
                                       Abstract* task)
{
    task->_context = std::move(context);

    task->run();

    task->_scheduler.pop();

    context = std::move(task->_context);

    return std::move(context);
}

Abstract::Abstract(scheduler::Abstract& scheduler) :
    _context   {entryPoint},
    _scheduler {scheduler}
{}

Abstract::~Abstract() {}

} // namespace task

} // namespace cosche