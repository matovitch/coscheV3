#pragma once

#include <boost/context/execution_context.hpp>

#include <functional>
#include <optional>
#include <future>

namespace cosche
{

namespace scheduler
{

struct Abstract;

} // namespace scheduler

template <std::size_t,
          std::size_t,
          std::size_t>
class TScheduler;

template<class RETURN_TYPE, class... ARGS>
class TTask;

namespace task
{

class Abstract
{
    template <std::size_t,
              std::size_t,
              std::size_t>
    friend class cosche::TScheduler;

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

template<class RETURN_TYPE, class... ARGS>
class TTask : public task::Abstract
{

public:

    TTask(scheduler::Abstract& scheduler) : task::Abstract{scheduler} {}

    void assign(std::function<RETURN_TYPE(ARGS...)>&& function, ARGS&&... args)
    {
        _functionOpt.emplace(std::bind(std::move(function), args...));
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

template<class... ARGS>
class TTask<void, ARGS...> : public task::Abstract
{

public:

    TTask(scheduler::Abstract& scheduler) : task::Abstract{scheduler} {}

    void assign(std::function<void(ARGS...)>&& function, ARGS&&... args)
    {
        _functionOpt.emplace(std::bind(std::move(function), args...));
    }

    void run()
    {
        if (!_functionOpt)
        {
            return;
        }

        (*_functionOpt)();
    }
    
private:

    std::optional<std::function<void()>> _functionOpt;
};

} // namespace cosche