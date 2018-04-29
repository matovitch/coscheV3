#pragma once

#include "likelyhood.hpp"

#include <cstdint>
#include <array>

namespace cosche
{

namespace pool
{

template <class       TYPE,
          std::size_t SIZE>
class TBuffer
{
    static constexpr std::size_t ALIGNOF = alignof(TYPE);
    static constexpr std::size_t  SIZEOF =  sizeof(TYPE);

public:

    TBuffer() : _tail{_data.data() + SIZE}
    {
        const std::size_t shift = reinterpret_cast<std::size_t>(_data.data()) % ALIGNOF;

        uint8_t* const dataPtr = _data.data();

        _head = (shift) ? dataPtr + ALIGNOF - shift
                        : dataPtr;

        for (auto&& byte : _data)
        {
            byte = 0;
        }
    }

    TYPE* allocate()
    {
        if (COSCHE_UNLIKELY(_tail - _head < SIZEOF))
        {
            return nullptr;
        }

        TYPE* const typePtr = reinterpret_cast<TYPE*>(_head);

        _head += SIZEOF;

        return typePtr;
    }

private:

    uint8_t*                           _head;
    std::array<uint8_t, SIZE * SIZEOF> _data;
    const uint8_t* const               _tail;
};

} // namespace pool

} // namespace cosche