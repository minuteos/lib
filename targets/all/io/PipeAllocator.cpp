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

    virtual void Destroy()
    {
        MemPoolFree<size>(this);
    }
};

class DynamicPipeSegment : PipeSegment
{
public:
    static PipeSegment* TryAlloc(size_t size, const uintptr_t*& mon)
    {
        if (auto mem = malloc(size + sizeof(DynamicPipeSegment)))
        {
            return new(mem) DynamicPipeSegment(size);
        }

        return NULL;
    }

private:
    DynamicPipeSegment(size_t size)
        : PipeSegment((const uint8_t*)(this + 1), size)
    {
    }

    virtual void Destroy()
    {
        free(this);
    }
};

class DefaultPipeAllocator : public PipeAllocator
{
    enum
    {
        PoolPayload64 = 64 - sizeof(PoolPipeSegment<64>),
        PoolPayloadMax = MEMPOOL_MAX_SIZE - sizeof(PoolPipeSegment<MEMPOOL_MAX_SIZE>),
        DynamicPayloadMax = 1024,
    };

public:
    virtual async(AllocateSegment, size_t hint, Timeout timeout)
    async_def(
        Timeout timeout;
    )
    {
        f.timeout = timeout.MakeAbsolute();

        for (;;)
        {
            const uintptr_t* mon;
            mon = NULL;
            if (auto seg = TryAllocateSegment(hint, mon))
            {
                async_return(intptr_t(seg));
            }

            MYTRACE("Failed to allocate segment, will try later");
            if (mon)
            {
                MYTRACE("Monitoring location %p", mon);
                if (!await_mask_not_timeout(*mon, ~0, *mon, f.timeout))
                {
                    async_return(0);
                }
            }
            else if (f.timeout.Elapsed())
            {
                async_return(0);
            }
            else
            {
                async_sleep_timeout(f.timeout);
            }
        }
    }
    async_end

private:
    static PipeSegment* TryAllocateSegment(size_t hint, const uintptr_t*& mon)
    {
        if (hint <= PoolPayload64)
        {
            return PoolPipeSegment<64>::TryAlloc(mon);
        }

        if (hint <= PoolPayloadMax)
        {
            return PoolPipeSegment<MEMPOOL_MAX_SIZE>::TryAlloc(mon);
        }

        if (hint > DynamicPayloadMax)
            hint = DynamicPayloadMax;

        return DynamicPipeSegment::TryAlloc(hint, mon);
    }
};

static DefaultPipeAllocator defaultAllocator;

PipeAllocator* PipeAllocator::s_default = &defaultAllocator;

}
