/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * Packed.cpp
 *
 * Tests the structure packing algorithm for various sizes of structures
 */

#include <testrunner/TestCase.h>

#include <base/Packed.h>

namespace   // prevent collisions
{

template<typename T> NO_INLINE Packed<T> noInlinePack(T& t) { return pack(t); }
template<typename T> NO_INLINE T noInlineUnpack(Packed<T>& t) { return unpack<T>(t); }

template<size_t n> struct S { uint32_t a[n]; };

template<size_t n> void Test(TestCase* t)
{
    S<n> s;
    for (unsigned i = 0; i < n; i++) { s.a[i] = i * 42; }
    auto packed = noInlinePack(s);
    auto unpacked = noInlineUnpack<S<n>>(packed);

    if (std::is_same<S<n>, Packed<S<n>>>::value)
        printf("Unsupported...");
    else
        printf("Supported...");

    for (unsigned i =0; i < n; i++)
        t->AssertEqual(s.a[i], unpacked.a[i]);
}

TEST_CASE("01 4-byte struct") { Test<1>(this); }
TEST_CASE("02 8-byte struct") { Test<2>(this); }
TEST_CASE("03 12-byte struct") { Test<3>(this); }
TEST_CASE("04 16-byte struct") { Test<4>(this); }
TEST_CASE("05 20-byte struct") { Test<5>(this); }
TEST_CASE("06 24-byte struct") { Test<6>(this); }

}
