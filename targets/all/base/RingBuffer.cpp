/*
 * Copyright (c) 2020 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/RingBuffer.cpp
 */

#include "RingBuffer.h"

//! Optimized for reasonable size-performance tradeoff on ARMv7m
ALWAYS_INLINE static void inline_memcpy(uint8_t*& dst, const uint8_t*& src, uint8_t* e)
{
    if (auto wordLength = (e - dst) & ~(sizeof(intptr_t) - 1))
    {
        uint8_t* ew = dst + wordLength;
        do
        {
            *(*(intptr_t**)&dst)++ = *(*(const intptr_t**)&src)++;
        } while (dst != ew);
    }

    while (dst != e)
    {
        *dst++ = *src++;
    }
}

res_pair_t RingBufferBase::AllocateImpl(size_t len)
{
    size_t required = Align(len);
    if (required >= Diff(write, read))
    {
        // won't fit
        return RingBufferAccessor();
    }

    // store original length
    auto w = write;
    *w++ = len;
    auto payload = (uint8_t*)w;
    write = (intptr_t*)Wrap(payload + required);

    return RingBufferAccessor(this, payload);
}

bool RingBufferBase::WriteImpl(const void* data, size_t len, uint8_t*& p, RingBufferBase* ring)
{
    if (!ring)
    {
        return false;
    }

    const uint8_t* src = (const uint8_t*)data;
    uint8_t* dst = p;
    size_t toEnd = ring->End() - dst;
    if (len > toEnd)
    {
        // copy to the end of buffer
        inline_memcpy(dst, src, dst + toEnd);
        dst = (uint8_t*)ring->data;
        len -= toEnd;
    }
    inline_memcpy(dst, src, dst + len);
    p = dst;
    return true;
}

res_pair_t RingBufferBase::DequeueImpl()
{
    if (read == write)
    {
        // nothing to dequeue
        return RingBufferAccessor();
    }

    auto r = read;
    size_t len = *r++;
    auto payload = (uint8_t*)r;
    read = (intptr_t*)Wrap(payload + Align(len));

    return RingBufferAccessor(this, payload);
}

res_pair_t RingBufferBase::ReadImpl(void* buffer, size_t len, uint8_t*& p, RingBufferBase* ring)
{
    if (!ring)
    {
        return Span();
    }

    const uint8_t* src = p;
    uint8_t* dst = (uint8_t*)buffer;
    size_t toEnd = ring->End() - src;
    if (len > toEnd)
    {
        // copy to the end of buffer
        inline_memcpy(dst, src, dst + toEnd);
        src = (uint8_t*)ring->data;
        len -= toEnd;
    }
    inline_memcpy(dst, src, dst + len);
    p = (uint8_t*)src;
    return Buffer(buffer, dst);
}