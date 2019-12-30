/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/PipeAllocator.cpp
 */

#include <io/PipeAllocator.h>
#include <io/PipeSegment.h>

#if PIPE_TRACE
#define MYTRACE(...)        DBGCL("pipealloc", __VA_ARGS__)
#else
#define MYTRACE(...)
#endif

namespace io
{

template<size_t size> class PoolPipeSegment : PipeSegment
{
public:
    static PipeSegment* TryAlloc(const uintptr_t*& mon)
    {
        MYTRACE("Allocating %d-byte mempool segment", size);
        if (auto mem = MemPoolAlloc<size>())
        {
            return new(mem) PoolPipeSegment<size>();
        }

        mon = MemPoolGet<size>()->WatchPointer();
        return NULL;
    }

private:
    PoolPipeSegment()
        : PipeSegment((const uint8_t*)(this + 1), size - sizeof(*this))
    {
    }

    virtual void Release()
    {
        MemPoolFree<size>(this);
    }
};

class DynamicPipeSegment : PipeSegment
{
public:
    static PipeSegment* TryAlloc(size_t min, size_t req, const uintptr_t*& mon)
    {
        size_t size = (req + 7) & ~7;
        if (auto mem = malloc(size))
        {
            return new(mem) DynamicPipeSegment(size);
        }

        return NULL;
    }

private:
    DynamicPipeSegment(size_t size)
        : PipeSegment((const uint8_t*)(this + 1), size - sizeof(*this))
    {
    }

    virtual void Release()
    {
        free(this);
    }
};

class DefaultPipeAllocator : public PipeAllocator
{
    enum
    {
        PoolSize32 = 32 - sizeof(PoolPipeSegment<32>),
        PoolSize64 = 64 - sizeof(PoolPipeSegment<64>),
        PoolSizeMax = MEMPOOL_MAX_SIZE - sizeof(PoolPipeSegment<MEMPOOL_MAX_SIZE>),
    };

public:
    virtual async(AllocateSegment, size_t min, size_t req, mono_t waitUntil)
    async_def()
    {
        for (;;)
        {
            const uintptr_t* mon;
            mon = NULL;
            if (auto seg = TryAllocateSegment(min, req, mon))
            {
                async_return(intptr_t(seg));
            }

            MYTRACE("Failed to allocate segment, will try later");
            if (mon)
            {
                MYTRACE("Monitoring location %p", mon);
                if (waitUntil)
                {
                    if (!await_mask_not_until(*mon, ~0, *mon, waitUntil))
                    {
                        async_return(0);
                    }
                }
                else
                {
                    await_mask_not(*mon, ~0, *mon);
                }
            }
            else if (waitUntil)
            {
                MYTRACE("Waiting for a while...");
                if (OVF_LE(waitUntil, MONO_CLOCKS))
                {
                    async_return(0);
                }

                async_sleep_until(waitUntil);
            }
            else
            {
                async_sleep_ms(100);
            }
        }
    }
    async_end

private:
    static PipeSegment* TryAllocateSegment(size_t min, size_t req, const uintptr_t*& mon)
    {
        if (req <= PoolSize32)
        {
            return PoolPipeSegment<32>::TryAlloc(mon);
        }

        if (req <= PoolSize64)
        {
            return PoolPipeSegment<64>::TryAlloc(mon);
        }

        if (req <= PoolSizeMax)
        {
            return PoolPipeSegment<MEMPOOL_MAX_SIZE>::TryAlloc(mon);
        }

        return DynamicPipeSegment::TryAlloc(min, req, mon);
    }
};

static DefaultPipeAllocator defaultAllocator;

PipeAllocator* PipeAllocator::s_default = &defaultAllocator;

}
