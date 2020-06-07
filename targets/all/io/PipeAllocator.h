/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/PipeAllocator.h
 */

#pragma once

#include <kernel/kernel.h>

namespace io
{

class PipeAllocator
{
public:
    virtual async(AllocateSegment, size_t min, size_t req, Timeout timeout) = 0;

private:
    static PipeAllocator* s_default;

    friend class Pipe;
};

}
