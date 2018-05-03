#pragma once

#include "buffer_supervisor.hpp"
#include "singleton.hpp"

#include <utility>
#include <vector>

namespace cosche
{

template <class FactoryTraits>
class TFactory
{
    using Type      = typename FactoryTraits::Type;
    using Allocator = typename FactoryTraits::Allocator;

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
            //reinterpret_cast<Type*>(ptr)->~Type();
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

namespace factory
{

template <class FactoryType>
struct TMakeTraits
{
    using Type      = FactoryType;
    using Allocator = buffer::supervisor::TMakeFromType<Type>;
};

template <class Type>
using TMake = TFactory<TMakeTraits<Type>>;

}


} // namespace cosche