/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * algo/MovingAverage.h
 */

#pragma once

#include <base/base.h>

template<typename T, size_t n> class MovingAverage
{
public:
    void Reset()
    {
        cnt = i = 0;
        agg = cur = {};
    }

    void Add(const T& element)
    {
        if (cnt == n)
        {
            // filter is full, overwrite oldest element
            agg -= values[i];
        }
        else
        {
            cnt++;
        }
        values[i] = element;
        agg += element;
        cur = agg / cnt;
        i = i < (n - 1) ? i + 1 : 0;
    }

    const T& Value() const { return cur; }
    operator const T&() const { return cur; }

private:
    T values[n];
    T agg = {}, cur = {};
    size_t cnt = 0, i = 0;
};

