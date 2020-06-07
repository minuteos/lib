/*
 * Copyright (c) 2020 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/tests/sanity/Timeout.cpp
 */

#include <testrunner/TestCase.h>

#include <kernel/Timeout.h>

namespace
{

using namespace kernel;

TEST_CASE("01 Infinite")
{
    Timeout t = Timeout::Infinite;
    Assert(t.IsInfinite());
    Assert(!t.IsAbsolute());
    Assert(t.IsRelative());
}

TEST_CASE("02 Absolute")
{
    auto t = Timeout::Absolute(MONO_CLOCKS + 10);
    Assert(!t.IsInfinite());
    Assert(t.IsAbsolute());
    Assert(!t.IsRelative());
}

TEST_CASE("03a Seconds")
{
    auto t = Timeout::Seconds(10);
    Assert(!t.IsInfinite());
    Assert(!t.IsAbsolute());
    Assert(t.IsRelative());
    AssertEqual(t.Relative(), (mono_signed_t)MonoFromSeconds(10));
}

TEST_CASE("03b Milliseconds")
{
    auto t = Timeout::Milliseconds(10);
    Assert(!t.IsInfinite());
    Assert(!t.IsAbsolute());
    Assert(t.IsRelative());
    AssertEqual(t.Relative(), (mono_signed_t)MonoFromMilliseconds(10));
}

TEST_CASE("03c Ticks")
{
    auto t = Timeout::Ticks(10);
    Assert(!t.IsInfinite());
    Assert(!t.IsAbsolute());
    Assert(t.IsRelative());
    AssertEqual(t.Relative(), 10);
}

TEST_CASE("04 MakeAbsolute")
{
    auto t = Timeout::Ticks(10);
    auto abs = t.MakeAbsolute();
    PLATFORM_SLEEP(0, 100);
    auto abs2 = t.MakeAbsolute();
    AssertNotEqual(abs, abs2);
}

TEST_CASE("05a CompareRelative")
{
    auto t1 = Timeout::Ticks(10), t2 = Timeout::Ticks(20);

    AssertLessThan(t1, t2);
    AssertLessOrEqual(t1, t2);
    AssertGreaterThan(t2, t1);
    AssertGreaterOrEqual(t2, t1);
    AssertNotEqual(t1, t2);
    Assert(!(t1 == t2));
    AssertEqual(t1, t1);
    Assert(!(t1 != t1));
}

TEST_CASE("05b CompareAbsolute")
{
    auto t1 = Timeout::Absolute(10), t2 = Timeout::Absolute(20);

    AssertLessThan(t1, t2);
    AssertLessOrEqual(t1, t2);
    AssertGreaterThan(t2, t1);
    AssertGreaterOrEqual(t2, t1);
    AssertNotEqual(t1, t2);
    Assert(!(t1 == t2));
    AssertEqual(t1, t1);
    Assert(!(t1 != t1));
}

TEST_CASE("05c CompareMixed A-R")
{
    auto t1 = Timeout::Absolute(10), t1r = Timeout::Ticks(10), t2 = Timeout::Ticks(20);

    AssertLessThan(t1, t2);
    AssertLessOrEqual(t1, t2);
    AssertGreaterThan(t2, t1);
    AssertGreaterOrEqual(t2, t1);
    AssertNotEqual(t1, t2);

    Assert(!(t1 < t1r));
    AssertLessOrEqual(t1, t1r);
    Assert(!(t1 > t1r));
    AssertGreaterOrEqual(t1, t1r);
    // they do not equal even if they compare properly
    Assert(!(t1 == t1r));
    AssertNotEqual(t1, t1r);
}

TEST_CASE("05d CompareMixed R-A")
{
    auto t1 = Timeout::Ticks(10), t1a = Timeout::Absolute(10), t2 = Timeout::Absolute(20);

    AssertLessThan(t1, t2);
    AssertLessOrEqual(t1, t2);
    AssertGreaterThan(t2, t1);
    AssertGreaterOrEqual(t2, t1);
    AssertNotEqual(t1, t2);
    Assert(!(t1 == t2));

    Assert(!(t1 < t1a));
    AssertLessOrEqual(t1, t1a);
    Assert(!(t1 > t1a));
    AssertGreaterOrEqual(t1, t1a);
    // they do not equal even if they compare properly
    Assert(!(t1 == t1a));
    AssertNotEqual(t1, t1a);
}

TEST_CASE("05e CompareInfinite")
{
    Timeout rel = Timeout::Ticks(10), abs = Timeout::Absolute(10), inf = Timeout::Infinite;

    AssertLessThan(rel, inf);
    AssertLessOrEqual(rel, inf);
    AssertGreaterThan(inf, rel);
    AssertGreaterOrEqual(inf, rel);
    AssertNotEqual(inf, rel);

    AssertLessThan(abs, inf);
    AssertLessOrEqual(abs, inf);
    AssertGreaterThan(inf, abs);
    AssertGreaterOrEqual(inf, abs);
    AssertNotEqual(inf, abs);
}

}
