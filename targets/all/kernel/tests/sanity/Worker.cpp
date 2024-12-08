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

}
