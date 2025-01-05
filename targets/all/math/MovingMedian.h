/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * algo/MovingAverage.h
 */

#pragma once

#include <base/base.h>

template<typename T, size_t n> class MovingMedian
{
public:
    void Reset()
    {
        cnt = i = 0;
    }

    void Add(const T& element)
    {
        int rm;
        if (cnt == n)
        {
            // filter is full, overwrite oldest element
            rm = GetIndex(values[i]);
        }
        else
        {
            // "remove" the element after the last one
            rm = cnt++;
        }
        values[i] = element;
        int ins = GetIndex(element);
        i = i < (n - 1) ? i + 1 : 0;

        if (rm < ins)
        {
            // move left
            memmove(&sorted[rm], &sorted[rm + 1], sizeof(T) * (ins - rm));
        }
        else if (rm > ins)
        {
            // move right
            memmove(&sorted[ins + 1], &sorted[ins], sizeof(T) * (rm - ins));
        }
        sorted[ins] = element;
        if (cnt & 1)
        {
            cur = sorted[cnt >> 1];
        }
        else
        {
            cur = (sorted[(cnt >> 1) - 1] + sorted[cnt >> 1]) / 2;
        }
    }

    const T& Value() const { return cur; }
    operator const T&() const { return cur; }

private:
    T values[n], sorted[n], cur;
    size_t cnt = 0, i = 0;

    int GetIndex(const T& value)
    {
        int s = 0, e = cnt - 1;
        while (s < e)
        {
            int m = (s + e) >> 1;
            if (value > sorted[m]) { s = m + 1; }
            else { e = m; }
        }
        return s;
    }
};

