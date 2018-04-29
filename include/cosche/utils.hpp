#pragma once

#include "node.hpp"
#include "task.hpp"

#include <functional>

namespace cosche
{

template <class RETURN_TYPE, class... ARGS>
void assignWork(TNode<task::Abstract*>& task,
                std::function<RETURN_TYPE(ARGS...)>&& work)
{
    reinterpret_cast<TTask<RETURN_TYPE, ARGS...>*>(task.value)->assign(std::move(work));
}

void cleanUp();

} // namespace cosche