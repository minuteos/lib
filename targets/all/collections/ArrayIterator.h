/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * collections/ArrayIterator.h
 *
 * Simple C++ iterator for consecutive elements (such as an array)
 */

#pragma once

#include <base/base.h>

template<typename T> struct ArrayIterator
{
    constexpr ArrayIterator(T* begin, T* end)
        : _begin(begin), _end(end) {}

    T* begin() const { return _begin; }
    T* end() const { return _end; }
    size_t size() const { return _end - _begin; }

private:
    T* _begin;
    T* _end;
};
