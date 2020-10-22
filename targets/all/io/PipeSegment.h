/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/PipeSegment.h
 */

#pragma once

#include <base/base.h>
#include <base/Span.h>

namespace io
{

class PipeSegment
{
protected:
    PipeSegment(const uint8_t* data, size_t length)
        : data(data), length(length), refs(0) {}

public:
    void Reference() { refs++; }
    bool Release() { if (!(refs--)) { Destroy(); return true; } return false; }

private:
    virtual void Destroy() = 0;

    bool Matches(size_t offset, Span data) const;

    PipeSegment* next = NULL;
    const uint8_t* data;
    uint16_t length;
    uint16_t refs;

    friend class Pipe;
};

}
