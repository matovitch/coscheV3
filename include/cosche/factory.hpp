#pragma once

#include "buffer_supervisor.hpp"
#include "singleton.hpp"

#include <utility>
#include <vector>

namespace cosche
{

namespace factory
{

template <class Type, std::size_t FIRST_BUFFER_SIZE = 1>
struct TMakeTraits
{
    using BufferSupervisorTraits = buffer::supervisor::TMakeTraitsFromType<Type, FIRST_BUFFER_SIZE>;
    using Type_ = Type;
};

} // namespace factory


template <class FactoryTraits>
class TFactory
{
    using Type                   = typename FactoryTraits::Type_;
    using BufferSupervisorTraits = typename FactoryTraits::BufferSupervisorTraits;
    using Allocator              = buffer::TSupervisor<BufferSupervisorTraits>;

public:

    TFactory() : 
        _allocator{TSingleton<Allocator>::instance()}
    {}

    template <class... ARGS>
    Type& make(ARGS&&... args)
    {
        void* ptr;

        if (!_recycleds.empty())
        {
            ptr = reinterpret_cast<void*>(_recycleds.back());
            _recycleds.pop_back();
            reinterpret_cast<Type*>(ptr)->~Type();
        }
        else
        {
            ptr = _allocator.allocateBlock();
        }

        new(ptr) Type(std::forward<ARGS>(args)...);

        return *(reinterpret_cast<Type*>(ptr));
    }

    void recycle(Type* const typePtr)
    {
        _recycleds.push_back(typePtr);
    }

private:

    Allocator&         _allocator;
    std::vector<Type*> _recycleds;
};


} // namespace cosche