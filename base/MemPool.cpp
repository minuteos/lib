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
    auto res = free;
    if (res)
    {
        free = res->next;
        res->next = NULL;
        return res;
    }
    return AllocNew();
}

void* MemPool::AllocDynamic()
{
    auto res = free;
    if (res)
    {
        free = res->next;
        res->pool = this;
        return res->data;
    }
    return AllocNewDynamic();
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
    void* mem = malloc_once(size);
#else
    void* mem = malloc(size);
#endif
    inline_memzero(mem, size);
    return mem;
}

void** MemPool::AllocLarge(size_t size)
{
    void* mem = malloc(size);
    inline_memzero(mem, size);
    return (void**)mem;
}

void MemPool::Free(void* mem)
{
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
    else
        free(ptr);
}
