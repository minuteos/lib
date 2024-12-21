/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/Transmitter.h
 *
 * An low-level strategy interface for a transmitter capable of transmitting
 * data from series of buffers, meant for outputting data from a Pipe
 */

#pragma once

#include <kernel/kernel.h>

#include <base/Span.h>

#include "PipeReader.h"

namespace io
{

class Transmitter
{
public:
    virtual ~Transmitter() {}

protected:
    //! Adds another buffer to the Transmitter, returns the number of bytes accepted for sending
    virtual size_t TryAddBlock(Span block) = 0;
    //! Gets the pointer to the next byte that is going to be sent
    virtual const char* GetReadPointer() = 0;
    //! Waits until the read pointer moves from the specified position
    virtual async_once(Wait, const char* current, Timeout timeout = Timeout::Infinite) = 0;

public:
    //! Meant to run as a task to transmit data from the specified pipe until it's closed
    async(TransmitFromPipe, PipeReader pipe);
    //! Starts the task to transmit data from the specified pipe until it's closed
    kernel::Task& StartTransmitFromPipe(PipeReader pipe)
    {
        return kernel::Task::Run(this, &Transmitter::TransmitFromPipe, pipe);
    }
};

}
