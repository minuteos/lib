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

}
