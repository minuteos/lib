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
            n = await(r.Require);
            AssertEqual(n, 4u);
            AssertEqual(r.Available(), 4u);
            AssertEqual(r.IsComplete(), true);
            Assert(r.Matches("TEST"));
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
            n = await(r.Require);
            AssertEqual(n, 2u);
            AssertEqual(r.Available(), 2u);
            AssertEqual(r.IsComplete(), false);
            Assert(r.Matches("TE"));
            n = await(r.Require, 3);
            AssertEqual(n, 4u);
            AssertEqual(r.Available(), 4u);
            AssertEqual(r.IsComplete(), true);
            Assert(r.Matches("TEST"));
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
            n = await(r.RequireUntil, '\n');
            AssertEqual(n, 7u);
            Assert(r.Matches("Line 1\n"));
            r.Advance(n);
            n = await(r.RequireUntil, '\n');
            AssertEqual(n, 0u);
            AssertEqual(r.IsComplete(), true);
            AssertEqual(r.Available(), 6u);
            Assert(r.Matches("Line 2")); // no terminator
            r.Advance(6);
        }
        async_end
    };

    s.Add(&S::Reader, p);
    s.Add(&S::Writer, p);
    s.Run();

    Assert(p.IsCompleted());
}


TEST_CASE("04 Copy")
{
    Scheduler s;
    Pipe p1;
    Pipe p2;

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
            n = await(r.RequireUntil, '\n');
            AssertEqual(n, 7u);
            Assert(r.Matches("Line 2\n"));
            r.Advance(n);
            n = await(r.RequireUntil, '\n');
            AssertEqual(n, 0u);
            AssertEqual(r.IsComplete(), true);
            AssertEqual(r.Available(), 6u);
            Assert(r.Matches("Line 1")); // no terminator
            r.Advance(6);
        }
        async_end

        static async(Copier, PipeReader p1, PipeWriter p2)
        async_def()
        {
            size_t n;
            n = await(p1.Require);
            AssertEqual(n, 13u);
            // mix things up for fun
            await(p1.CopyTo, p2, 7, 6);
            await(p1.CopyTo, p2, 6, 1);
            await(p1.CopyTo, p2, 0, 6);
            p2.Close();
            p1.Advance(13);
        }
        async_end
    };

    s.Add(&S::Writer, p1);
    s.Add(&S::Reader, p2);
    s.Add(&S::Copier, p1, p2);
    s.Run();

    Assert(p1.IsCompleted());
    Assert(p2.IsCompleted());
}

TEST_CASE("04 Move")
{
    Scheduler s;
    Pipe p1;
    Pipe p2;

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
            n = await(r.RequireUntil, '\n');
            AssertEqual(n, 7u);
            Assert(r.Matches("Line 1\n"));
            r.Advance(n);
            n = await(r.RequireUntil, '\n');
            AssertEqual(n, 0u);
            AssertEqual(r.IsComplete(), true);
            AssertEqual(r.Available(), 6u);
            Assert(r.Matches("Line 2")); // no terminator
            r.Advance(6);
        }
        async_end

        static async(Mover, PipeReader p1, PipeWriter p2)
        async_def()
        {
            size_t n;
            n = await(p1.Require);
            AssertEqual(n, 13u);
            await(p1.MoveTo, p2, 13);
            p2.Close();
        }
        async_end
    };

    s.Add(&S::Writer, p1);
    s.Add(&S::Reader, p2);
    s.Add(&S::Mover, p1, p2);
    s.Run();

    Assert(p1.IsCompleted());
    Assert(p2.IsCompleted());
}

}
