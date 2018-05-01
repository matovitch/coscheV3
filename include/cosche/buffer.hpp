#pragma once

#include "likelyhood.hpp"

#include <cstdint>
#include <memory>
#include <array>

namespace cosche
{

namespace buffer
{

static constexpr std::size_t MAX_SIZE = 1 << 0x10;

template <class AbstractTraits>
struct TAbstract
{
    virtual void* allocateBlock() = 0;

    virtual std::unique_ptr<TAbstract<AbstractTraits>> makeBufferNext() const = 0;

    virtual ~TAbstract() {}
};

template <class Type>
struct TMakeAbstractTraitsFromType
{
    static constexpr std::size_t BLOCK_SIZE  =  sizeof(Type);
    static constexpr std::size_t BLOCK_ALIGN = alignof(Type);
};

template <class Type, std::size_t BUFFER_SIZE = 1>
struct TMakeTraitsFromType
{
    static constexpr std::size_t SIZE = BUFFER_SIZE + 1;

    using AbstractTraits = TMakeAbstractTraitsFromType<Type>;
    using NextTraits     = TMakeTraitsFromType<Type, std::min(SIZE << 1, MAX_SIZE)>;
};

} // namespace buffer


template <class BufferTraits>
class TBuffer : public buffer::TAbstract<typename BufferTraits::AbstractTraits>
{
    using BufferAbstractTraits = typename BufferTraits::AbstractTraits;
    using BufferNextTraits     = typename BufferTraits::NextTraits;
    
    static constexpr std::size_t SIZEOF    = BufferAbstractTraits::BLOCK_SIZE;
    static constexpr std::size_t ALIGNOF   = BufferAbstractTraits::BLOCK_ALIGN;
    static constexpr std::size_t BYTE_SIZE = BufferTraits::SIZE * SIZEOF;

    using BufferAbstract = buffer::TAbstract<BufferAbstractTraits>;
    using BufferNext     = TBuffer<BufferNextTraits>;

public:

    TBuffer() : _tail{_data.data() + BYTE_SIZE}
    {
        const std::size_t shift = reinterpret_cast<std::size_t>(_data.data()) % ALIGNOF;

        uint8_t* const dataPtr = _data.data();

        _head = (shift) ? dataPtr + ALIGNOF - shift
                        : dataPtr;
    }

    void* allocateBlock() override
    {
        if (COSCHE_UNLIKELY(_tail - _head < SIZEOF))
        {
            return nullptr;
        }

        void* const typePtr = reinterpret_cast<void*>(_head);

        _head += SIZEOF;

        return typePtr;
    }

    virtual std::unique_ptr<BufferAbstract> makeBufferNext() const override
    {
        return std::make_unique<BufferNext>();
    }

private:

    uint8_t*                       _head;
    std::array<uint8_t, BYTE_SIZE> _data;
    const uint8_t* const           _tail;
};

} // namespace cosche