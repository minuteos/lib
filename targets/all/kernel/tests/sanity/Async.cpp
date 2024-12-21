/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/tests/sanity/Async.cpp
 *
 * Direct tests of asynchronous function calling mechanisms
 */

#include <testrunner/TestCase.h>

#include <kernel/async.h>
#include <kernel/Scheduler.h>

namespace   // prevent collisions
{

TEST_CASE("01 Simple call")
{
    struct
    {
        async(Test, int ms, int tick, int res) async_def()
        {
            async_sleep_ms(ms);
            async_sleep_ticks(tick);
            async_delay_ms(ms);
            async_delay_ticks(tick);
            async_return(res);
        }
        async_end

        AsyncFrame* p = NULL;
        async_res_t Step() { return Test(&p, 10, 20, 30); }
    } t;

    auto res = t.Step();
    AssertEqual(_ASYNC_RES_TYPE(res), AsyncResult::SleepMilliseconds);
    AssertEqual(_ASYNC_RES_VALUE(res), 10);
    AssertNotEqual(t.p, (AsyncFrame*)NULL);

    res = t.Step();
    AssertEqual(_ASYNC_RES_TYPE(res), AsyncResult::SleepTicks);
    AssertEqual(_ASYNC_RES_VALUE(res), 20);

    res = t.Step();
    AssertEqual(_ASYNC_RES_TYPE(res), AsyncResult::DelayMilliseconds);
    AssertEqual(_ASYNC_RES_VALUE(res), 10);

    res = t.Step();
    AssertEqual(_ASYNC_RES_TYPE(res), AsyncResult::DelayTicks);
    AssertEqual(_ASYNC_RES_VALUE(res), 20);

    res = t.Step();
    AssertEqual(_ASYNC_RES_TYPE(res), AsyncResult::Complete);
    AssertEqual(_ASYNC_RES_VALUE(res), 30);
    AssertEqual(t.p, (AsyncFrame*)NULL);
}

TEST_CASE("02 Masked waits")
{
    static auto test = []()
    {
        struct
        {
            async(Test, int ms, int tick, int res) async_def()
            {
                if (await_mask_sec(x, 1, 1, 3))
                {
                    if (!await_mask_not_ms(x, 4, 0, 6))
                    {
                        await_mask(x, 0xFF, 0); // should pass immediately
                    }
                }
            }
            async_end

            int x = 0;
            AsyncFrame* p = NULL;
            async_res_t Step() { return Test(&p, 10, 20, 30); }
        } t;

        auto res = t.Step();
        AssertNotEqual(t.p, (AsyncFrame*)NULL);
        AssertEqual(_ASYNC_RES_TYPE(res), AsyncResult::Wait);
        AssertEqual((AsyncFrame*)_ASYNC_RES_VALUE(res), t.p);
        AssertEqual((int*)t.p->waitPtr, &t.x);
        AssertEqual(unsafe_cast<Timeout>(t.p->waitTimeout), Timeout::Seconds(3u));
        t.p->waitResult = true;  // simulate success

        res = t.Step();
        AssertEqual(_ASYNC_RES_TYPE(res), AsyncResult::WaitInverted);
        AssertEqual((AsyncFrame*)_ASYNC_RES_VALUE(res), t.p);
        AssertEqual((int*)t.p->waitPtr, &t.x);
        AssertEqual(unsafe_cast<Timeout>(t.p->waitTimeout), Timeout::Milliseconds(6));
        t.p->waitResult = false; // simulate timeout

        res = t.Step();
        AssertEqual(_ASYNC_RES_TYPE(res), AsyncResult::Complete);
        AssertEqual(_ASYNC_RES_VALUE(res), 0);
        AssertEqual(t.p, (AsyncFrame*)NULL);
    };

    kernel::Scheduler s;
    s.Add([](AsyncFrame** p)
    {
        test();
        async_once_return(0);
    });
    s.Run();
}

}
