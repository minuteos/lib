/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 */

#include <testrunner/TestCase.h>

int main(int argc, char **argv)
{
    return TestCase::RunAll(argc - 1, argv + 1);
}
