/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/Receiver.h
 *
 * An low-level strategy interface for a receiver capable of filling multiple
 * buffers, meant for pushing data into a Pipe
 */

#pragma once

#include <kernel/kernel.h>

#include <base/Span.h>

#include "PipeWriter.h"

namespace io
{

class Receiver
{
public:
    virtual ~Receiver() {}

protected:
    //! Tries to add another buffer or a part of the buffer to the receiver
    //! @returns the number of bytes that have been added successfully
    virtual size_t TryAddBuffer(size_t offset, Buffer buffer) = 0;
    //! Gets the current write position in one of the buffers
    virtual const char* GetWritePointer(Buffer buffer) = 0;
    //! Waits until the write pointer moves from the specified position
    virtual async_once(Wait, const char* current, Timeout timeout = Timeout::Infinite) = 0;
    //! Stop the reader, any added buffers may be deallocated after returning
    virtual async_once(Close) = 0;

public:
    //! Meant to run as a task to fill the specified pipe until it's closed
    async(ReceiveToPipe, PipeWriter pipe, size_t blockHint = 1);
    //! Starts the task to fill the specified pipe until it's closed
    kernel::Task& StartReceiveToPipe(PipeWriter pipe, size_t blockHint = 1)
    {
        return kernel::Task::Run(this, &Receiver::ReceiveToPipe, pipe, blockHint);
    }
};

}
