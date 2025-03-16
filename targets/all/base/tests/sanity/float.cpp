/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/tests/sanity/float.cpp
 */

#include <testrunner/TestCase.h>

#include <base/float.h>
#include <base/format.h>

namespace
{

static const struct
{
    float f;
    const char* s;
} spec[] = {
    { 1, "1" },
    { 0.1, "0.1" },
    { 10, "10" },
    { 100, "100" },
    { 1.07, "1.07" },
    { 0.097, "0.097" },
    { 1e-36, "1e-36" },
    { -1.03443575e-36, "-1.03443575e-36" },
    // some known problematic conversions that popped up during development
    { unsafe_cast<float>(1), "1e-45" },
    { unsafe_cast<float>(7), "1e-44" },
    { unsafe_cast<float>(0x01688B8F), "4.2711799e-38" },
    { unsafe_cast<float>(0x01688B90), "4.2711801e-38" },
    { unsafe_cast<float>(0x12F029A7), "1.51563974e-27" },
    { unsafe_cast<float>(0xE000000), "1.5777218e-30" },
    { unsafe_cast<float>(0xBE800000), "-0.25" },
    { unsafe_cast<float>(0x6C000000), "6.1897002e26" },
    { unsafe_cast<float>(0x6F800000), "7.9228162e28" },
    { unsafe_cast<float>(0x01E881F1), "8.54098e-38" },
    { unsafe_cast<float>(0x1778A4A5), "8.034093e-25" },
    { unsafe_cast<float>(0x1C79CFFC), "8.2656e-22" },
};

TEST_CASE("01 FtoA")
{
    char buf[17];
    buf[16] = '!';

    for (auto s: spec)
    {
        *fast_ftoa(s.f, buf) = 0;
        AssertEqualString(buf, s.s);
    }

    AssertEqual(buf[16], '!');
}

TEST_CASE("02 Roundtrip")
{
    for (auto s: spec)
    {
        float f = fast_atof(s.s);
        AssertEqual(f, s.f);
    }
}

TEST_CASE("03 AtoF out of range")
{
    AssertEqual(fast_atof("1e60"), INFINITY);
    AssertEqual(fast_atof("1e-60"), 0);
    AssertEqual(fast_atof("-1e60"), -INFINITY);
    Assert(isnan(fast_atof("not a number")));
}

TEST_CASE("P1 FtoA 100k conversions")
{
    char buf[16];
    for (int i = 0; i < 1000000; i++)
    {
        fast_ftoa(unsafe_cast<float>(i), buf);
    }
}

TEST_CASE("P2 FtoA+AtoF 100k conversions")
{
    char buf[16];
    for (int i = 0; i < 1000000; i++)
    {
        fast_ftoa(unsafe_cast<float>(i), buf);
        fast_atof(buf);
    }
}


TEST_CASE("P3.1 format 100k overhead")
{
    for (int i = 0; i < 1000000; i++)
    {
        format(format_output_discard, NULL, "");
    }
}

TEST_CASE("P3.2 format 100k itoa")
{
    for (int i = 0; i < 1000000; i++)
    {
        format(format_output_discard, NULL, "%d", i);
    }
}

}
