/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/Transmitter.cpp
 *
 * Implements the logic of transmitting data from a pipe using a strategy
 * provided by the actual implementation.
 */

#include "Transmitter.h"

namespace io
{

async(Transmitter::TransmitFromPipe, PipeReader pipe)
async_def(
    Transmitter* self;
    PipeReader pipe;
    PipePosition pos;
    bool monitoring;

    void Monitor()
    {
        if (!monitoring && TryAdvance())
        {
            // run a background monitoring task if there are still data to be sent
            monitoring = true;
            kernel::Task::Run(this, &__FRAME::MonitorTask);
        }
    }

    //! Advances the pipe by the amount of data already sent
    //! @returns the pointer to the next byte to be sent or NULL if all data has been sent
    const char* TryAdvance()
    {
        for (;;)
        {
            auto span = pipe.GetSpan();
            if (!span.Length())
            {
                // no more data to send
                return NULL;
            }
            auto p = self->GetReadPointer();
            // if the pointer is inside the buffer, data up to it is already sent and we need to wait until it moves
            if (p >= span.begin() && p <= span.end())
            {
                pipe.Advance(p - span.begin());
                return p;
            }

            // otherwise the entire buffer has already been sent and we need to get another one
            pipe.Advance(span.Length());
        }
    }

    async(MonitorTask)
    async_def()
    {
        while (auto p = TryAdvance())
        {
            async_suppress_uninitialized_warning(
                await(self->Wait, p)
            );
        }
        monitoring = false;
    }
    async_end
)
{
    f.self = this;
    f.pipe = pipe;
    f.pos = pipe.Position();

    for (;;)
    {
        auto span = pipe.GetSpanAt(f.pos);
        if (!span.Length())
        {
            if (pipe.IsComplete())
            {
                // pipe complete
                break;
            }
        }
        else
        {
            size_t accepted = TryAddBlock(span);
            if (accepted)
            {
                f.pos += accepted;
                f.Monitor();
                continue;
            }
        }

        await(pipe.Change);
    }

    await_signal_off(f.monitoring);
}
async_end

}
