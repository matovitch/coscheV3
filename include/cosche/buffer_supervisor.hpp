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

namespace supervisor
{

template <class Type, std::size_t FIRST_BUFFER_SIZE = 1>
struct TMakeTraitsFromType
{
    using BufferTraits = buffer::TMakeTraitsFromType<Type, FIRST_BUFFER_SIZE>;
};

} // namespace supervisor

template <class SupervisorTraits>
class TSupervisor
{
    using BufferConcreteTraits = typename SupervisorTraits::BufferTraits;
    using BufferAbstractTraits = typename BufferConcreteTraits::AbstractTraits;

    using BufferConcrete = TBuffer<BufferConcreteTraits>;
    using BufferAbstract = buffer::TAbstract<BufferAbstractTraits>;

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

} // namespace buffer

} // namespace cosche