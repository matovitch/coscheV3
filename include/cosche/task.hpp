#pragma once

#include "boost/context/execution_context.hpp"
#include "singleton.hpp"
#include "factory.hpp"

#include <functional>
#include <optional>
#include <future>

namespace cosche
{

class Scheduler;

namespace scheduler
{

struct Abstract;

} // namespace scheduler

namespace task
{

class Abstract
{
    friend class cosche::Scheduler;

public:

    Abstract(scheduler::Abstract& scheduler);

    virtual void run() = 0;

    virtual ~Abstract();

private:

    using Context = boost::context::execution_context<Abstract*>;

    static Context entryPoint(Context&& context, Abstract* const task);

    Context              _context;
    scheduler::Abstract& _scheduler;

};

} // namespace task

template<class RETURN_TYPE>
class TTask : public task::Abstract
{

public:

    TTask(scheduler::Abstract& scheduler) : task::Abstract{scheduler} {}

    void assign(std::function<RETURN_TYPE()>&& function)
    {
        _functionOpt.emplace(std::move(function));
    }

    void run()
    {
        if (!_functionOpt)
        {
            return;
        }

        _promise.set_value((_functionOpt.value())());
    }

    std::future<RETURN_TYPE> future()
    {
        return _promise.getFuture();
    }

private:

    std::optional<std::function<RETURN_TYPE()>> _functionOpt;
    std::promise<RETURN_TYPE>                   _promise;
};


template<>
class TTask<void> : public task::Abstract
{

public:

    TTask(scheduler::Abstract& scheduler) : task::Abstract{scheduler} {}

    void assign(std::function<void()>&& function)
    {
        _functionOpt.emplace(std::move(function));
    }

    void run()
    {
        if (!_functionOpt)
        {
            return;
        }

        (*_functionOpt)();

        using TaskFactoryTraits = factory::TMakeTraits<TTask<void>>;
        using TaskFactory       = TFactory<TaskFactoryTraits>;

        TSingleton<TaskFactory>::instance().recycle(this);
    }

private:

    std::optional<std::function<void()>> _functionOpt;
};

} // namespace cosche