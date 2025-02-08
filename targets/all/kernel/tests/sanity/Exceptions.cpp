/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/tests/sanity/Exceptions.cpp
 *
 * Tests for throwing exceptions in async functions
 */

#include <testrunner/TestCase.h>

#include <kernel/async.h>
#include <kernel/Scheduler.h>

namespace   // prevent collisions
{

TEST_CASE("01 Simple throw")
async_test
{
    NO_INLINE async(Run)
    async_def()
    {
        auto res = await_catch(Throw, 42);
        AssertEqual(res.ExceptionType(), kernel::Error);
        AssertEqual(res.Value(), 42);

        res = await_catch(ThrowOnce, 43);
        AssertEqual(res.ExceptionType(), kernel::Error);
        AssertEqual(res.Value(), 43);

        res = await_catch(ThrowOnceDef, 44);
        AssertEqual(res.ExceptionType(), kernel::Error);
        AssertEqual(res.Value(), 44);

        res = await_catch(NoThrow, 45);
        Assert(res.Success());
        AssertEqual(res.Value(), 45);
    }
    async_end

    NO_INLINE async(Throw, intptr_t value)
    async_def_sync()
    {
        async_throw(kernel::Error, value);
    }
    async_end

    NO_INLINE async_once(ThrowOnce, intptr_t value)
    {
        async_once_throw(kernel::Error, value);
    }

    NO_INLINE async_once(ThrowOnceDef, intptr_t value)
    async_once_def()
    {
        async_throw(kernel::Error, value);
    }
    async_end

    NO_INLINE async_once(NoThrow, intptr_t value)
    {
        async_once_return(value);
    }
}
async_test_end

static char buf[10];
static char* p;

TEST_CASE("02 Throw Sequence")
async_test
{
    async(Run)
    async_def()
    {
        p = buf;
        auto res = await_catch(A);
        *p = 0;
        AssertEqual(res.ExceptionType(), kernel::Error);
        AssertEqual(res.Value(), 0);
        AssertEqualString(buf, "ABCcba");
    }
    async_end

    async(A)
    async_def(
        ~__FRAME() { *p++ = 'a'; }
    )
    {
        *p++ = 'A';
        await(B);
        *p++ = 'X';
    }
    async_end

    async(B)
    async_def(
        ~__FRAME() { *p++ = 'b'; }
    )
    {
        *p++ = 'B';
        await(C);
        *p++ = 'X';
    }
    async_end

    async(C)
    async_def_sync(
        ~__FRAME() { *p++ = 'c'; }
    )
    {
        *p++ = 'C';
        async_throw(kernel::Error, 0);
        *p++ = 'X';
    }
    async_end
}
async_test_end

}
