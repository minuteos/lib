/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/PipeSegment.h
 */

#pragma once

#include <base/base.h>

namespace io
{

class PipeSegment
{
protected:
    PipeSegment(const uint8_t* data, size_t length)
        : data(data), length(length) {}

private:
    virtual void Release() = 0;

    PipeSegment* next = NULL;
    const uint8_t* data;
    size_t length;

    friend class Pipe;
};

}
