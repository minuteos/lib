/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/tests/pipes/Pipes.cpp
 */

#include <testrunner/TestCase.h>

#include <kernel/kernel.h>
#include <io/io.h>

namespace
{

using namespace io;
using namespace kernel;

TEST_CASE("01 Simple Pipe")
{
    Scheduler s;
    Pipe p;

    struct S
    {
        static async(Writer, PipeWriter w)
        async_def()
        {
            await(w.Write, "TEST");
            w.Close();
        }
        async_end

        static async(Reader, PipeReader r)
        async_def()
        {
            size_t n;
            n = await(r.Read);
            AssertEqual(n, 4u);
            AssertEqual(r.Available(), 4u);
            AssertEqual(r.IsComplete(), true);
            AssertEqual(r.FirstSpan(), Span("TEST"));
            r.Advance(n);
        }
        async_end
    };

    s.Add(&S::Reader, p);
    s.Add(&S::Writer, p);
    s.Run();

    Assert(p.IsCompleted());
}

TEST_CASE("02 Multi Write")
{
    Scheduler s;
    Pipe p;

    struct S
    {
        static async(Writer, PipeWriter w)
        async_def()
        {
            await(w.Write, "TE");
            async_yield();
            await(w.Write, "ST");
            w.Close();
        }
        async_end

        static async(Reader, PipeReader r)
        async_def()
        {
            size_t n;
            n = await(r.Read);
            AssertEqual(n, 2u);
            AssertEqual(r.Available(), 2u);
            AssertEqual(r.IsComplete(), false);
            AssertEqual(r.FirstSpan(), Span("TE"));
            r.Examined(n);
            n = await(r.Read);
            AssertEqual(n, 4u);
            AssertEqual(r.Available(), 4u);
            AssertEqual(r.IsComplete(), true);
            AssertEqual(r.FirstSpan(), Span("TEST"));
            r.Advance(n);
        }
        async_end
    };

    s.Add(&S::Reader, p);
    s.Add(&S::Writer, p);
    s.Run();

    Assert(p.IsCompleted());
}

TEST_CASE("03 Read Until")
{
    Scheduler s;
    Pipe p;

    struct S
    {
        static async(Writer, PipeWriter w)
        async_def()
        {
            await(w.Write, "Line 1\nLine 2");
            w.Close();
        }
        async_end

        static async(Reader, PipeReader r)
        async_def()
        {
            size_t n;
            n = await(r.ReadUntil, '\n');
            AssertEqual(n, 7u);
            AssertEqual(r.FirstSpan().Left(n), Span("Line 1\n"));
            r.Advance(n);
            n = await(r.ReadUntil, '\n');
            AssertEqual(n, 0u);
            AssertEqual(r.IsComplete(), true);
            AssertEqual(r.Available(), 6u);
            AssertEqual(r.FirstSpan(), Span("Line 2")); // no terminator
            r.Advance(6);
        }
        async_end
    };

    s.Add(&S::Reader, p);
    s.Add(&S::Writer, p);
    s.Run();

    Assert(p.IsCompleted());
}

}
