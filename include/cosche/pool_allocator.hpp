#pragma once

#include "pool_buffer.hpp"

#include <memory>
#include <vector>

namespace cosche
{

namespace pool
{

template <class       TYPE,
          std::size_t BUFFER_SIZE>
class TAllocator
{
    using Buffer = TBuffer<TYPE, BUFFER_SIZE>;

public:

    TAllocator()
    {
        _buffers.emplace_back(std::make_unique<Buffer>());
    }

    void recycle(TYPE* const typePtr)
    {
        _recycleds.push_back(typePtr);
    }


    TYPE* allocate()
    {
        if (!_recycleds.empty())
        {
            TYPE* const typePtr = _recycleds.back();
            _recycleds.pop_back();
            typePtr->~TYPE();
            return typePtr;
        }

        TYPE* const typePtr = _buffers.back()->allocate();

        if (COSCHE_UNLIKELY(typePtr == nullptr))
        {
            _buffers.emplace_back(std::make_unique<Buffer>());
            allocate();
        }

        return typePtr;
    }

private:

    std::vector<TYPE*>                   _recycleds;
    std::vector<std::unique_ptr<Buffer>> _buffers;
};

} // namespace pool

} // namespace cosche