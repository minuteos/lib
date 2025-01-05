/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * algo/median.h
 *
 * Simple median implementations for fixed number of elements
 */

#pragma once

#include <base/base.h>

template<typename T> T median(T a, T b, T c)
{
    return a > b
        ? (c > a ? a : c > b ? c : b)
        : (c > b ? b : c > a ? c : a);
}

template<typename T> T median(T a, T b, T c, T d, T e)
{
    T t;
    if (a > b) { t = a; a = b; b = t; }
    // a < b
    if (c > d) { t = c; c = d; d = t; }
    // c < d
    if (a > c) { t = b; b = d; d = t; c = a; }
    // b < c < d, a eliminated
    if (e > b) { a = b; b = e; } else { a = e; }
    // a < b
    if (c > a) { t = b; b = d; d = t; a = c; }

    return a < d ? a : d;
}
