#pragma once

#include "scheduler.hpp"
#include "node.hpp"
#include "task.hpp"

#include <functional>

namespace cosche
{

template <class RETURN_TYPE, class... ARGS>
void assignWork(TNode<task::Abstract*>& task,
                std::function<RETURN_TYPE(ARGS...)>&& work,
                ARGS&&... args)
{
    reinterpret_cast<TTask<RETURN_TYPE, ARGS...>*>(task.value)->assign(std::move(work), std::move(args)...);
}

void cleanUp();

using Scheduler = TScheduler<scheduler::TMakeTraits<>>;

} // namespace cosche