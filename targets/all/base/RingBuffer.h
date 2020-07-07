/*
 * Copyright (c) 2020 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/RingBuffer.h
 *
 * Simple ring buffer for variable-length messages
 */

#pragma once

#include <base/base.h>
#include <base/Span.h>

class RingBufferBase
{
protected:
    RingBufferBase(intptr_t* buffer, size_t size)
        : read(buffer), write(buffer), data(buffer), size(size) {}

    intptr_t* read;
    intptr_t* write;
    intptr_t* data;
    size_t size;

    //! Aligns the size to word boundary
    static constexpr size_t Align(size_t size) { return (size + sizeof(intptr_t) - 1) & ~(sizeof(intptr_t) - 1); }
    //! Returns the number of words required to store @param size bytes
    static constexpr size_t Words(size_t size) { return (size + sizeof(intptr_t) - 1) / sizeof(intptr_t); }
    //! Calculates the difference between two pointers, wrapping around the ring
    constexpr size_t Diff(intptr_t* low, intptr_t* high) const
    {
        return high > low ?
            (intptr_t)high - (intptr_t)low :
            (intptr_t)high - (intptr_t)low + size;
    }
    //! Gets the pointer to the end of buffer
    constexpr uint8_t* End() const { return (uint8_t*)data + size; }
    //! Wraps a pointer so that it stays inside the buffer
    template<typename T> constexpr T* Wrap(T* ptr)
    {
        ASSERT(!((intptr_t)ptr & (sizeof(T) - 1)));
        return ptr < End() ? ptr : (T*)((uint8_t*)ptr - size);
    }

    RES_PAIR_DECL(AllocateImpl, size_t length);
    static bool WriteImpl(const void* data, size_t length, uint8_t*& p, RingBufferBase* ring);
    RES_PAIR_DECL(DequeueImpl);
    static RES_PAIR_DECL(ReadImpl, void* data, size_t length, uint8_t*& p, RingBufferBase* ring);

    friend class RingBufferAccessor;
    friend class RingBufferReader;
    friend class RingBufferWriter;
};

class RingBufferAccessor
{
    union
    {
        struct
        {
            RingBufferBase* ring;
            uint8_t* p;
        };
        res_pair_t pair;
    };

    ALWAYS_INLINE constexpr RingBufferAccessor() : ring(NULL), p(NULL) {}
    ALWAYS_INLINE constexpr RingBufferAccessor(RingBufferBase* ring, uint8_t* payload) : ring(ring), p(payload) {}
    ALWAYS_INLINE constexpr RingBufferAccessor(res_pair_t pair) : pair(pair) {}

    ALWAYS_INLINE constexpr operator res_pair_t() const { return pair; }

    ALWAYS_INLINE constexpr size_t ExtractLength() const { return p ? ((intptr_t*)p)[-1] : 0; }

public:
    constexpr operator bool() const { return ring; }

    friend class RingBufferBase;
    friend class RingBufferReader;
    friend class RingBufferWriter;
};

class RingBufferReader : public RingBufferAccessor
{
    size_t length;

    ALWAYS_INLINE constexpr RingBufferReader(res_pair_t pair)
        : RingBufferAccessor(pair), length(ExtractLength()) {}

public:
    constexpr size_t Available() const { return length; }

    ALWAYS_INLINE Buffer Read(Buffer data)
    {
        Buffer res = RingBufferBase::ReadImpl(data.Pointer(), std::min(length, data.Length()), p, ring);
        length -= res.Length();
        return res;
    }

    template<size_t> friend class RingBuffer;
};

class RingBufferWriter : public RingBufferAccessor
{
    size_t length;

    ALWAYS_INLINE constexpr RingBufferWriter(res_pair_t pair)
        : RingBufferAccessor(pair), length(ExtractLength()) {}

public:
    constexpr size_t Available() const { return length; }

    //! Writes data to the buffer, returns @ref true on success
    ALWAYS_INLINE bool Write(Span data)
    {
        if (!ring) return false;
        auto len = std::min(length, data.Length());
        length -= len;
        return RingBufferBase::WriteImpl(data.Pointer(), len, p, ring);
    }

    template<size_t> friend class RingBuffer;
};

template<size_t TSize> class RingBuffer : RingBufferBase
{
public:
    RingBuffer() : RingBufferBase(_data, sizeof(_data)) {}

    //! Checks if the buffer is empty
    constexpr bool IsEmpty() const { return read == write; }
    //! Number of bytes used
    constexpr size_t Used() { return sizeof(data) - Diff(write, read); }
    //! Number of bytes available in the buffer (maximum record size)
    constexpr size_t Available() { return Diff(write + 2, read); }

    //! Allocates an item in the buffer, @returns a manipulator for storing item data. Data must be written before yielding control.
    ALWAYS_INLINE RingBufferWriter Allocate(size_t length) { return AllocateImpl(length); }
    //! Retrieves an item from the buffer, @returns a manipulator for reading item data. Data must be read before yielding control.
    ALWAYS_INLINE RingBufferReader Dequeue() { return DequeueImpl(); }

    //! Appends an item to the buffer, @returns true on success
    ALWAYS_INLINE bool Enqueue(Span record) { return Allocate(record.Length()).Write(record); }
    //! Retrieves an item from the buffer, @returns buffer with adjusted length, or an invalid @ref Buffer if empty
    ALWAYS_INLINE Buffer Dequeue(Buffer buffer) { return Dequeue().Read(buffer); }

private:
    intptr_t _data[Words(TSize)];
};
