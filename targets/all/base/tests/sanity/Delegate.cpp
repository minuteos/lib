/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * Delegate.cpp
 *
 * Tests of the Delegate decoding algorithm for various pointer-to-member
 * cases
 */

#include <testrunner/TestCase.h>

#include <base/Delegate.h>

namespace   // prevent collisions
{

struct A
{
    int basis = 10;
    NO_INLINE int GetValue() { return basis - 9; }    // 1
    virtual int GetVirtual() { return basis - 8; }  // 2
};

struct A2 : A
{
    int basis = 100;
    virtual int GetVirtual() override { return basis - 97; }    // 3
};

struct B
{
    int basis = 1000;
    NO_INLINE int GetValue2() { return basis - 996; } // 4
    virtual int GetVirtual2() { return basis - 995; }   // 5
};

struct AB : A, B
{
};

TEST_CASE("01 Simple member")
{
    A a;
    auto d = GetDelegate(&a, &A::GetValue);
    AssertEqual(1, d());
}

TEST_CASE("02 Virtual member")
{
    A a;
    auto d = GetDelegate(&a, &A::GetVirtual);
    AssertEqual(2, d());
}

TEST_CASE("03 Overriden member")
{
    A2 a;
    auto d = GetDelegate((A*)&a, &A::GetVirtual);
    AssertEqual(3, d());
}

TEST_CASE("04 Multiple inheritance")
{
    AB ab;
    // the cast is required for some reason, as the compiler considers &AB::GetValue2 as a pointer to a member of B, not AB
    auto d = GetDelegate(&ab, (int(AB::*)())&AB::GetValue2);
    AssertEqual(4, d());
}

TEST_CASE("05 Multiple inheritance virtual")
{
    AB ab;
    // the cast is required for some reason, as the compiler considers &AB::GetVirtual2 as a pointer to a member of B, not AB
    auto d = GetDelegate(&ab, (int(AB::*)())&AB::GetVirtual2);
    AssertEqual(5, d());
}

}
