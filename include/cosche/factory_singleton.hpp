#pragma once

#include "singleton.hpp"
#include "factory.hpp"

namespace cosche
{

template <class Type>
using TFactorySingleton = TSingleton<factory::TMake<Type>>;

} // namespace cosche