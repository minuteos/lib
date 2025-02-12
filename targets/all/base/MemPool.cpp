/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * MemPool.cpp
 *
 * Simple fixed-size low-overhead memory pools
 */

#include <base/MemPool.h>

#include <base/alloc_trace.h>

// inline helper for zeroing aligned memory
// note that it is MANDATORY for size to be non-zero and a multiple of natural pointer size (intptr_t)
ALWAYS_INLINE static void inline_memzero(void* ptr, size_t size)
{
    auto p = (intptr_t*)ptr;
    auto e = (intptr_t*)((intptr_t)p + size);
    do
    {
        *p++ = 0;
    } while (p < e);
}

void* MemPool::Alloc()
{
#if MEMPOOL_DEBUG_PERIODIC_DUMP
    cnt++;
#endif
    auto res = free;
    if (res)
    {
        free = res->next;
        res->next = NULL;
        __trace_alloc(res, size);
        return res;
    }
    auto ptr = AllocNew();
    __trace_alloc(ptr, size);
    return ptr;
}

void* MemPool::AllocDynamic()
{
#if MEMPOOL_DEBUG_PERIODIC_DUMP
    cnt++;
#endif
    auto res = free;
    if (res)
    {
        free = res->next;
        res->pool = this;
        __trace_alloc(res, size);
        return res->data;
    }
    auto ptr = AllocNewDynamic();
    __trace_alloc((char*)ptr - sizeof(MemPool*), size);
    return ptr;
}

void* MemPool::AllocNewDynamic()
{
    auto res = (MemPoolEntry*)AllocNew();
    res->pool = this;
    return res->data;
}

void* MemPool::AllocNew()
{
    // simply allocate the required memory, it will go back into the pool once freed
#if HAS_MALLOC_ONCE
    void* mem = (char*)malloc_once(ALLOC_TRACE_OVERHEAD + size) + ALLOC_TRACE_OVERHEAD;
#else
    void* mem = malloc(size);
#endif
    inline_memzero(mem, size);
    return mem;
}

#if !MEMPOOL_NO_MALLOC
void** MemPool::AllocLarge(size_t size)
{
    void* mem = malloc(size);
    inline_memzero(mem, size);
    return (void**)mem;
}
#endif

void MemPool::Free(void* mem)
{
    __trace_free(mem);
#if MEMPOOL_DEBUG_PERIODIC_DUMP
    cnt--;
    if (MONO_CLOCKS - lastDump >= MONO_FREQUENCY)
    {
        lastDump += MONO_FREQUENCY;
        DBGL("MP %d = %d", size, cnt);
    }
#endif
    // enqueuing the chunk to the freelist before zeroing allows the ARM version
    // of this function to get by with only the 4 scratch registers,
    // avoiding stack push/pop
    *(MemPoolEntry**)mem = free;
    free = (MemPoolEntry*)mem;
    inline_memzero((MemPoolEntry**)mem + 1, size - sizeof(MemPoolEntry*));
}

void MemPoolFreeDynamic(void* mem)
{
    auto ptr = (class MemPool**)mem - 1;
    if (*ptr)
        (*ptr)->Free(ptr);
#if !MEMPOOL_NO_MALLOC
    else
        free(ptr);
#endif
}
