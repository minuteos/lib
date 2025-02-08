/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/tests/sanity/Edge.cpp
 *
 * Tests for edge case scenarios
 */

#include <testrunner/TestCase.h>

#include <kernel/kernel.h>

namespace   // prevent collisions
{

TEST_CASE("01 Call after waitresult")
async_test
{
    async(Run)
    async_def()
    {
        auto res = await(kernel::Task::Switch, GetMethodDelegate(this, Return1));
        AssertEqual(res, 1);
        await(Call);

        auto xres = await_catch(kernel::Task::Switch, GetMethodDelegate(this, Throw));
        AssertException(xres, kernel::Error, 1);
        await(Call);
    }
    async_end

    async(Call)
    async_def()
    {
        async_delay_ms(10);
        Assert(true);
    }
    async_end

    async(Return1)
    async_def_sync()
    {
        async_return(1);
    }
    async_end

    async(Throw)
    async_def_sync()
    {
        async_throw(kernel::Error, 1);
    }
    async_end
}
async_test_end

}
