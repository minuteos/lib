/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * MemPool.cpp
 * 
 * Tests the fixed-size pool allocator
 * 
 * Only very rudimentary tests are carried out (essentially only compilability
 * of all code paths is tested), as it's quite hard to create a simple test that
 * would prove anything, and many other tests would fail in case of a bug
 * in the allocator anyway.
 */

#include <testrunner/TestCase.h>

#include <base/MemPool.h>

namespace   // prevent collisions
{

TEST_CASE("01 Fixed alloc")
{
    auto mem = MemPoolAlloc<int>();
    AssertEqual(*mem, 0);   // memory must be zeroed
    MemPoolFree(mem);

    auto mem2 = MemPoolAlloc<int>();
    AssertEqual(mem, mem2); // same block must be returned immediately after freeing
    MemPoolFree(mem2);

    auto memLarge = MemPoolAlloc<int8_t[MEMPOOL_MAX_SIZE * 2]>();
    MemPoolFree(memLarge);
}

TEST_CASE("02 Dynamic alloc")
{
    int* mem = MemPoolAllocDynamic<int>();
    MemPoolFreeDynamic(mem);

    auto mem2 = MemPoolAllocDynamic<int8_t[MEMPOOL_MAX_SIZE * 2]>();
    MemPoolFreeDynamic(mem2);
}

TEST_CASE("03 MemPoolGet")
{
    // maximum size MemPool must still be available
    AssertNotEqual(MemPoolGet<MEMPOOL_MAX_SIZE>(), (MemPool*)NULL);
    // size above maximum must not be available
    AssertEqual(MemPoolGet<MEMPOOL_MAX_SIZE * 2>(), (MemPool*)NULL);
    // check smallest possible mempool
    AssertEqual(MemPoolGet<1>(), MemPoolGet<MEMPOOL_MIN_SIZE>());
}

}
