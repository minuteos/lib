/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * math/tests/filters/MovingMedian.cpp
 */

#include <testrunner/TestCase.h>

#include <math/MovingMedian.h>

TEST_CASE("Median")
{
    MovingMedian<float, 5> mm {};

    AssertEqual(mm.Value(), 0);
    mm.Add(1);
    AssertEqual(mm.Value(), 1);
    mm.Add(2);
    AssertEqual(mm.Value(), 1.5);
    mm.Add(0);
    AssertEqual(mm.Value(), 1);
    mm.Add(2);
    AssertEqual(mm.Value(), 1.5);
    mm.Add(2);
    AssertEqual(mm.Value(), 2);
    mm.Add(2);
    AssertEqual(mm.Value(), 2);
    mm.Add(1);
    AssertEqual(mm.Value(), 2);
    mm.Add(0);
    AssertEqual(mm.Value(), 2);
    mm.Add(0);
    AssertEqual(mm.Value(), 1);
}
