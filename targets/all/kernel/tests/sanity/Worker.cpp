/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/tests/sanity/Worker.cpp
 */

#include <testrunner/TestCase.h>

#include <kernel/kernel.h>

namespace   // prevent collisions
{

TEST_CASE("01a Function Worker")
async_test
{
    static int Add(int a, int b) { return a + b; }

    async(Run)
    async_def()
    {
        AssertEqual(await(kernel::Worker::Run, Add, 13, 17), 30);
    }
    async_end
}
async_test_end

TEST_CASE("01b Delegate worker")
async_test
{
    int x;
    int AddX(int a) { return a + x; }

    async(Run)
    async_def()
    {
        x = 2;
        AssertEqual(await(kernel::Worker::Run, GetMethodDelegate(this, AddX), 40), 42);
    }
    async_end
}
async_test_end

static int x;

TEST_CASE("01c Void Function Worker")
async_test
{
    static void SetX(int val) { x = val; }

    async(Run)
    async_def()
    {
        AssertEqual(await(kernel::Worker::Run, SetX, 38), 0);
        AssertEqual(x, 38);
    }
    async_end
}
async_test_end

TEST_CASE("01d Void Delegate Worker")
async_test
{
    int x;
    void SetX(int val) { this->x = val; }

    async(Run)
    async_def()
    {
        AssertEqual(await(kernel::Worker::Run, GetMethodDelegate(this, SetX), 34), 0);
        AssertEqual(x, 34);
    }
    async_end
}
async_test_end

TEST_CASE("02 Await in Worker")
async_test
{
    mono_t t1w, t2w, t3w;
    mono_t t1, t2, t3, t4;

    void Worker()
    {
        t1w = MONO_CLOCKS;
        kernel::Worker::Await(GetMethodDelegate(this, AsyncWorker), 100);
        t2w = MONO_CLOCKS;
        kernel::Worker::Await(GetMethodDelegate(this, AsyncOnceWorker), 50);
        t3w = MONO_CLOCKS;
    }

    async(AsyncWorker, mono_t delay)
    async_def()
    {
        t1 = MONO_CLOCKS;
        async_delay_ticks(delay);
        t2 = MONO_CLOCKS;
    }
    async_end

    async_once(AsyncOnceWorker, mono_t delay)
    async_once_def()
    {
        t3 = MONO_CLOCKS;
        async_delay_ticks(delay);
        t4 = MONO_CLOCKS;
    }
    async_end

    async(Run)
    async_def()
    {
        t4 = 0;

        await(kernel::Worker::Run, GetMethodDelegate(this, Worker));

        AssertEqual(t1, t1w);
        AssertEqual(t2, t2w);
        AssertEqual(t2 - t1, 100u);
        AssertEqual(t3w - t2w, 50u);
        AssertEqual(t3, t2);
        AssertEqual(t4, 0u);
    }
    async_end
}
async_test_end


}
