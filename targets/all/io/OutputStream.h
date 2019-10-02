/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/OutputStream.h
 *
 * Base class for all output streams
 */

#pragma once

#include <kernel/kernel.h>

#include <base/Span.h>

namespace io
{

class OutputStream
{
public:
    virtual async(Write, Span span, unsigned msTimeout = 0) = 0;
};

}
