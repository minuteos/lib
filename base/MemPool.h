/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * MemPool.h
 * 
 * Simple fixed-size low-overhead memory pools
 */

#pragma once

#include <base/base.h>

// minimal pool size is at least two pointers
// this is so that we're able to avoid checks for zero-length when zeroing returned chunks
#define MEMPOOL_MIN_SIZE        (2 * sizeof(intptr_t))

#ifndef MEMPOOL_MAX_SIZE
// maximum pool size - larger chunks are malloc'd directly
#define MEMPOOL_MAX_SIZE        (32 * sizeof(intptr_t))
#endif

#ifndef MEMPOOL_GRANULARITY
#define MEMPOOL_GRANULARITY     (2 * sizeof(intptr_t))
#endif

struct MemPoolEntry
{
    union
    {
        MemPoolEntry* next;
        class MemPool* pool;
    };
    uint8_t data[];
};

class MemPool
{
    MemPoolEntry* free;
    const size_t size;

public:
    MemPool(const size_t size) : free(NULL), size(size)
    {
    }

    void* Alloc();
    void Free(void* block);

private:
    void* AllocDynamic();
    void* AllocNew();
    void* AllocNewDynamic();

    template<size_t> friend void* MemPoolAlloc();
    template<size_t> friend void* MemPoolAllocDynamic();

    static void** AllocLarge(size_t size);
};

// calculates the size of the pool to be used for an arbitrary size/type
template<size_t size> constexpr size_t MemPoolSize()
{
    return size > MEMPOOL_MAX_SIZE ? 0 :
        ((size < MEMPOOL_MIN_SIZE ? MEMPOOL_MIN_SIZE : size) + MEMPOOL_GRANULARITY - 1) & ~(MEMPOOL_GRANULARITY - 1);
}

template<typename T> constexpr size_t MemPoolSize() { return MemPoolSize<sizeof(T)>(); }

template<size_t size> class __MemPoolInstance
{
    static class MemPool s_instance;

    template<size_t> friend void* MemPoolAlloc();
    template<size_t> friend void* MemPoolAllocDynamic();
    template<size_t> friend void MemPoolFree(void*);
};

template<size_t size> class MemPool __MemPoolInstance<size>::s_instance(size);

template<size_t size> ALWAYS_INLINE void* MemPoolAlloc()
{
    if (size > MEMPOOL_MAX_SIZE)
        return MemPool::AllocLarge(size);
    else
        return __MemPoolInstance<MemPoolSize<size>()>::s_instance.Alloc();
}
template<typename T> ALWAYS_INLINE T* MemPoolAlloc() { return (T*)MemPoolAlloc<sizeof(T)>(); }

template<size_t size> ALWAYS_INLINE void MemPoolFree(void* ptr)
{
    if (size > MEMPOOL_MAX_SIZE)
        free(ptr);
    else
        __MemPoolInstance<MemPoolSize<size>()>::s_instance.Free(ptr);
}
template<typename T> ALWAYS_INLINE void MemPoolFree(T* entry) { return MemPoolFree<sizeof(T)>(entry); }

template<size_t size> ALWAYS_INLINE void* MemPoolAllocDynamic()
{
    constexpr auto poolSize = size + sizeof(class MemPool*);
    if (poolSize > MEMPOOL_MAX_SIZE)
    {
        // leaving the pool pointer NULL means memory was allocated dynamically
        return MemPool::AllocLarge(poolSize) + 1;
    }
    else
    {
        return __MemPoolInstance<MemPoolSize<size>()>::s_instance.AllocDynamic();
    }
}
template<typename T> ALWAYS_INLINE T* MemPoolAllocDynamic() { return (T*)MemPoolAllocDynamic<sizeof(T)>(); }

void MemPoolFreeDynamic(void* mem);
