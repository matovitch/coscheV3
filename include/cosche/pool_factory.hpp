#pragma once

#include "pool_allocator.hpp"
#include "singleton.hpp"

#include <utility>

namespace cosche
{

namespace pool
{

template <class       TYPE,
          std::size_t ALLOCATOR_BUFFER_SIZE>
class TFactory
{
    using Allocator = TAllocator<TYPE, ALLOCATOR_BUFFER_SIZE>;

public:

    TFactory() : 
        _allocator{Singleton<Allocator>::instance()}
    {}

    template <class... ARGS>
    TYPE& make(ARGS&&... args)
    {
        TYPE* const typePtr = _allocator.allocate();

        new(static_cast<void *>(typePtr)) TYPE(std::forward<ARGS>(args)...);

        return *typePtr;
    }

    void recycle(TYPE* const typePtr)
    {
        _allocator.recycle(typePtr);
    }

private:

    Allocator& _allocator;
};

} // namespace pool

} // namespace cosche
