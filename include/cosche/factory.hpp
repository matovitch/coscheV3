#pragma once

#include "buffer_supervisor.hpp"
#include "singleton.hpp"

#include <utility>
#include <vector>

namespace cosche
{

template <class FactoryType>
class TFactory
{
    using Type      = FactoryType;
    using Allocator = buffer::supervisor::TMakeFromType<Type>;

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
            destroy(ptr);
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

    ~TFactory()
    {
        _allocator.clean(&destroy);
    }

private:

    static void destroy(void* ptr)
    {
        reinterpret_cast<Type*>(ptr)->~Type();
    }

    Allocator&         _allocator;
    std::vector<Type*> _recycleds;
};

} // namespace cosche