/*
 * Copyright (c) 2020 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/tests/sanity/PeriodicWakeup.cpp
 */

#include <testrunner/TestCase.h>

#include <kernel/PeriodicWakeup.h>

namespace
{

using namespace kernel;

TEST_CASE("01 Simple")
{
    Scheduler s;

    struct Test
    {
        static async(Task) async_def(mono_t start; PeriodicWakeup wake = PeriodicWakeup(3, 700))
        {
            f.start = MONO_CLOCKS;
            await(f.wake.Next);
            AssertNotEqual(f.wake.Error(), 0u);
            await(f.wake.Next);
            AssertNotEqual(f.wake.Error(), 0u);
            await(f.wake.Next);
            AssertEqual(f.wake.Error(), 0u);
            AssertEqual(MONO_CLOCKS - f.start, 700u);
        }
        async_end
    };

    s.Add(Test::Task);
    s.Run();
}

}
