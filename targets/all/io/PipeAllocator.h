/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/PipeAllocator.h
 */

#pragma once

#include <kernel/kernel.h>

#include <io/Errors.h>

namespace io
{

class PipeAllocator
{
public:
    //! Allocates a new segment, throws if timeout expires before a new segment can be allocated
    virtual async(AllocateSegment, size_t hint, Timeout timeout) = 0;

private:
    static PipeAllocator* s_default;

    friend class Pipe;
};

}
