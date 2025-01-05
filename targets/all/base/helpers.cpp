/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/helpers.cpp
 */

#include "helpers.h"

int parse_nibble(char c, int base)
{
    if (c < '0') return -1;
    if (c <= '9') return c - '0';
    if (c < 'A') return -1;
    if (c < 'A' + base - 10) return c - 'A' + 10;
    if (c < 'a') return -1;
    if (c < 'a' + base - 10) return c - 'a' + 10;
    return -1;
}

int f2q(float f, unsigned decimals)
{
    int mul = 1;
    while (decimals--) { mul *= 10; }
    f *= mul;
    if (signbit(f)) { f -= 0.5f; }
    else { f += 0.5f; }
    return f;
}
