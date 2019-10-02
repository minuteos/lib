/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/InputStream.h
 *
 * Base class for all input streams
 */

#pragma once

#include <kernel/kernel.h>

#include <base/Span.h>

namespace io
{

class InputStream
{
public:
    virtual async(Read, Buffer buffer, unsigned msTimeout = 0) = 0;
};

}
