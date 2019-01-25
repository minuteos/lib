/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/Buffer.h
 * 
 * Helper for working with a contiguous block of writable memory
 */

#pragma once 

#include <base/base.h>

class Buffer : public Span
{
public:
    //! Constructs an empty (invalid) buffer
    constexpr Buffer() {}
    //! Constructs a Buffer covering a range of memory determined by start and length
    constexpr Buffer(void* p, size_t len) : Span(p, len) {}
    //! Constructs a Buffer covering a range of memory determined by start (inclusive) and end (exclusive)
    constexpr Buffer(void* start, void* end) : Span(start, end) {}
    //! Constructs a Buffer covering a single object
    template<class T> constexpr Buffer(T& value) : Span((char*)&value, sizeof(T)) {}

};

