#pragma once

#include "likelyhood.hpp"

#include <functional>
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

    virtual void clean(const std::function<void(void*)>& destructor) = 0;

    virtual ~TAbstract() {}
};

} // namespace buffer


template <class BufferTraits>
class TBuffer : public buffer::TAbstract<typename BufferTraits::Abstract>
{
    static constexpr std::size_t SIZEOF    = BufferTraits::Abstract::BLOCK_SIZE;
    static constexpr std::size_t ALIGNOF   = BufferTraits::Abstract::BLOCK_ALIGN;
    static constexpr std::size_t BYTE_SIZE = BufferTraits::SIZE * SIZEOF;

    using BufferNext = TBuffer<typename BufferTraits::Next>;

public:

    using BufferAbstract = buffer::TAbstract<typename BufferTraits::Abstract>;

    TBuffer() : _tail{_data.data() + BYTE_SIZE}
    {
        _head = pointerToFirstBlock();
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

    void clean(const std::function<void(void*)>& destructor) override
    {
        uint8_t* head = pointerToFirstBlock();

        while (head != _head)
        {
            destructor(head);
            head += SIZEOF;
        }
    }

private:

    uint8_t* pointerToFirstBlock()
    {
        const std::size_t shift = reinterpret_cast<std::size_t>(_data.data()) % ALIGNOF;

        uint8_t* const dataPtr = _data.data();

        return (shift) ? dataPtr + ALIGNOF - shift
                       : dataPtr;
    }

    uint8_t*                       _head;
    std::array<uint8_t, BYTE_SIZE> _data;
    const uint8_t* const           _tail;
};

namespace buffer
{

template <std::size_t BUFFER_BLOCK_SIZE,
          std::size_t BUFFER_BLOCK_ALIGN>
struct AbstractTraits
{
    static constexpr std::size_t BLOCK_SIZE  = BUFFER_BLOCK_SIZE;
    static constexpr std::size_t BLOCK_ALIGN = BUFFER_BLOCK_ALIGN;
};

template <std::size_t BUFFER_BLOCK_SIZE,
          std::size_t BUFFER_BLOCK_ALIGN,
          std::size_t BUFFER_SIZE>
struct TTraits
{
    static constexpr std::size_t SIZE = BUFFER_SIZE + 1;

    using Abstract = AbstractTraits<BUFFER_BLOCK_SIZE,
                                    BUFFER_BLOCK_ALIGN>;

    using Next = TTraits<BUFFER_BLOCK_SIZE,
                         BUFFER_BLOCK_ALIGN,
                         std::min(SIZE << 1, MAX_SIZE)>;
};

template <class Type>
using TMakeTraitsFromType = TTraits< sizeof(Type),
                                    alignof(Type), 1>;

template <class Type>
using TMakeFromType = TBuffer<TMakeTraitsFromType<Type>>;

} // namespace buffer

} // namespace cosche