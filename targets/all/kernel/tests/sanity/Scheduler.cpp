/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/tests/sanity/Scheduler.cpp
 *
 * Basic task scheduler tests
 */

#include <testrunner/TestCase.h>

#include <kernel/kernel.h>

namespace   // prevent collisions
{

using namespace kernel;

struct SequenceRecorder
{
    char buf[1024];
    char* mark = buf;

protected:
    void Mark(char m)
    {
        mark += sprintf(mark, "%s%c@%lu", mark == buf ? "" : ",", m, (long)MonoToMilliseconds(MONO_CLOCKS));
    }

public:
    operator const char*() const { return buf; }
};

TEST_CASE("01 Simple task")
{
    Scheduler s;

    struct Test : SequenceRecorder
    {
        async(Task) async_def()
        {
            Mark('a');
            async_delay_ms(10);
            Mark('b');
            async_delay_ms(10);
            Mark('c');
        }
        async_end
    } t;

    s.Add(t, &Test::Task);
    auto endTime = s.Run();

    AssertEqualString(t, "a@0,b@10,c@20");
    AssertGreaterOrEqual(endTime, MonoFromMilliseconds(20));
    AssertLessThan(endTime, MonoFromMilliseconds(21));
}

TEST_CASE("02 Static task")
{
    static mono_t a, b;
    Scheduler s;

    struct Test
    {
        static async(Task) async_def()
        {
            a = MONO_CLOCKS;
            async_delay_ms(10);
            b = MONO_CLOCKS;
        }
        async_end
    };

    s.Add(Test::Task);
    auto endTime = s.Run();
    AssertEqual(a, 0u);
    AssertEqual(b, MonoFromMilliseconds(10));
    AssertEqual(endTime, b);
}

TEST_CASE("03 Interleaved tasks")
{
    struct Test : SequenceRecorder
    {
        async(Task1) async_def()
        {
            Mark('1');
            async_delay_ms(8);
            Mark('1');
            async_sleep_ms(20);  // the sleep will be interrupted by the delay in Task2, and this task will run before task2
            Mark('1');
            async_delay_ms(4);
            Mark('1');
        }
        async_end

        async(Task2) async_def()
        {
            Mark('2');
            async_delay_ms(5);
            Mark('2');
            async_delay_ms(10);
            Mark('2');
            async_delay_ms(5);
            Mark('2');
        }
        async_end
    } t;

    Scheduler s;
    s.Add(t, &Test::Task1);
    s.Add(t, &Test::Task2);
    auto endTime = s.Run();

    AssertEqualString(t, "1@0,2@0,2@5,1@8,1@15,2@15,1@19,2@20");
    AssertGreaterOrEqual(endTime, MonoFromMilliseconds(20));
    AssertLessThan(endTime, MonoFromMilliseconds(21));
}

TEST_CASE("04 Masked waits")
{
    struct Test : SequenceRecorder
    {
        uint32_t x = 0;

        async(Task1) async_def()
        {
            x |= 1;
            if (await_mask_ms(x, 2, 2, 100))
            {
                Mark('1');
            }
            else
            {
                Mark('X');
            }
            x &= ~1;
        }
        async_end

        async(Task2) async_def()
        {
            async_delay_ms(10);
            x |= 2;
            Mark('2');
        }
        async_end

        async(Task3) async_def()
        {
            if (await_mask_not_ms(x, 1, 1, 5))
            {
                Mark('X');
            }
            else
            {
                Mark('3');
            }
        }
        async_end
    } t;

    Scheduler s;
    s.Add(t, &Test::Task1);
    s.Add(t, &Test::Task2);
    s.Add(t, &Test::Task3);
    auto endTime = s.Run();

    AssertEqualString(t, "3@5,2@10,1@10");
    AssertGreaterOrEqual(endTime, MonoFromMilliseconds(10));
    AssertLessThan(endTime, MonoFromMilliseconds(11));
}

TEST_CASE("04b Signal waits")
{
    struct Test : SequenceRecorder
    {
        uint8_t signal[4] = { 0 };

        async(Task1) async_def()
        {
            signal[1] = 1;
            if (await_signal_ms(signal[2], 100))
            {
                Mark('1');
            }
            else
            {
                Mark('X');
            }
            signal[1] = 0;
        }
        async_end

        async(Task2) async_def()
        {
            async_delay_ms(10);
            signal[2] = 1;
            Mark('2');
        }
        async_end

        async(Task3) async_def()
        {
            if (await_signal_off_ms(signal[1], 5))
            {
                Mark('X');
            }
            else
            {
                Mark('3');
            }
        }
        async_end
    } t;

    Scheduler s;
    s.Add(t, &Test::Task1);
    s.Add(t, &Test::Task2);
    s.Add(t, &Test::Task3);
    auto endTime = s.Run();

    AssertEqualString(t, "3@5,2@10,1@10");
    AssertGreaterOrEqual(endTime, MonoFromMilliseconds(10));
    AssertLessThan(endTime, MonoFromMilliseconds(11));
}

TEST_CASE("05 Async calls")
{
    struct Test : SequenceRecorder
    {
        AsyncDelegate<int> b;

        async(Task) async_def()
        {
            Mark('T');
            await(A);
            Mark('T');
        }
        async_end

        async(A) async_def()
        {
            Mark('A');
            async_delay_ms(10);
            await(b, 10);
            Mark('A');
        }
        async_end

        async(B, int delay) async_def()
        {
            Mark('B');
            async_delay_ms(delay);
            Mark('B');
        }
        async_end
    } t;

    Scheduler s;
    s.Add(t, &Test::Task);
    t.b = GetDelegate(&t, &Test::B);
    auto endTime = s.Run();

    AssertEqualString(t, "T@0,A@0,B@10,B@20,A@20,T@20");
    AssertGreaterOrEqual(endTime, MonoFromMilliseconds(20));
    AssertLessThan(endTime, MonoFromMilliseconds(21));
}

}
