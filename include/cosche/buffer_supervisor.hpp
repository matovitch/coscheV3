#pragma once

#include "likelyhood.hpp"
#include "buffer.hpp"

#include <functional>
#include <cstdint>
#include <memory>
#include <vector>
#include <array>

namespace cosche
{

namespace buffer
{

template <class SupervisorTraits>
class TSupervisor
{
    using BufferConcrete = typename SupervisorTraits::BufferConcrete;
    using BufferAbstract = typename SupervisorTraits::BufferAbstract;

public:

    TSupervisor()
    {
        _buffers.emplace_back(std::make_unique<BufferConcrete>());
    }

    void* allocateBlock()
    {
        void* const blockPtr = _buffers.back()->allocateBlock();

        if (COSCHE_UNLIKELY(blockPtr == nullptr))
        {
            _buffers.emplace_back(_buffers.back()->makeBufferNext());
            return _buffers.back()->allocateBlock();
        }

        return blockPtr;
    }

    void clean(const std::function<void(void*)>& destructor)
    {
        for (auto&& buffer : _buffers)
        {
            buffer->clean(destructor);
        }
    }

private:

    std::vector<std::unique_ptr<BufferAbstract>> _buffers;
};

namespace supervisor
{

template <class BufferType>
struct TTraits
{
    using BufferConcrete =          BufferType;
    using BufferAbstract = typename BufferType::BufferAbstract;
};

template <class Type>
using TMakeTraitsFromType = TTraits<buffer::TMakeFromType<Type>>;

template <class Type>
using TMakeFromType = TSupervisor<TMakeTraitsFromType<Type>>;

} // namespace supervisor

} // namespace buffer

} // namespace cosche