/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/tests/sanity/ID.cpp
 */

#include <base/base.h>
#include <base/ID.h>

#include <testrunner/TestCase.h>

namespace
{

TEST_CASE("01 Byte Order")
{
    ID id("TEST");
    Assert(!memcmp("TEST", &id, 4));
}

TEST_CASE("02 Alternate Lengths")
{
    ID id0(""), id1("T"), id2("TE"), id3("TES");
    AssertEqual(0u, (uint32_t)id0);
    Assert(!memcmp("T\0\0\0", &id1, 4));
    Assert(!memcmp("TE\0\0\0", &id2, 4));
    Assert(!memcmp("TES\0\0", &id3, 4));
}

}
