/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/Receiver.cpp
 *
 * Implements the logic of pushing data into a pipe using a strategy provided
 * by the actual implementation.
 */

#include "Receiver.h"

namespace io
{

async(Receiver::ReceiveToPipe, PipeWriter pipe, size_t blockHint)
async_def(
    Receiver* self;
    PipeWriter pipe;
    PipePosition pos;
    size_t blockHint;
    bool allocating;

    void Allocate()
    {
        if (!allocating && TryAddBuffers())
        {
            allocating = true;
            kernel::Task::Run(this, &__FRAME::AllocateTask);
        }
    }

    //! Tries to add more buffers to the receivers synchronously
    //! @returns true if more buffers need to be allocated
    bool TryAddBuffers()
    {
        for (;;)
        {
            auto buf = pipe.GetBufferAt(pos);
            if (!buf.Length())
            {
                // need to allocate more buffers
                return pipe.CanAllocate();
            }

            // make sure all received data is accounted for before trying to add new buffer
            // this is needed for strategies that copy data from their own buffers
            // during GetWritePointer
            TryAdvance();

            size_t added = self->TryAddBuffer(pos - pipe.Position(), buf);
            if (!added)
            {
                // receiver cannot take more buffers
                return false;
            }
            pos += added;
        }
    }

    const char* TryAdvance()
    {
        while (size_t buffered = pipe.Position() - pos)
        {
            auto buf = pipe.GetBuffer().Left(buffered);
            if (!buf.Length())
            {
                // no buffer, nothing to wait for, need to allocate
                break;
            }

            auto p = self->GetWritePointer(buf);
            // if the pointer is inside the buffer, data up to it is filled in
            // otherwise the entire buffer is already full
            size_t available = p >= buf.begin() && p < buf.end() ? p - buf.begin() : buf.Length();
            if (!available)
            {
                return p;
            }

            pipe.Advance(available);
        }

        return NULL;
    }

    async(AllocateTask)
    async_def()
    {
        while (TryAddBuffers())
        {
            // allocate a new block
            if (!await(pipe.Allocate, blockHint))
            {
                break;
            }
        }

        allocating = false;
    }
    async_end
)
{
    f.self = this;
    f.pipe = pipe;
    f.pos = pipe.Position();
    f.blockHint = blockHint;

    f.Allocate();

    while (!pipe.IsClosed())
    {
        f.TryAdvance();
        f.Allocate();

        if (auto p = f.TryAdvance())
        {
            async_suppress_uninitialized_warning(
                await(Wait, p);
            );
        }
        else
        {
            // no buffers, we need to wait for the pipe to move forward
            if (f.allocating)
            {
                // wait for the allocation to complete
                await(pipe.Change);
            }
            else
            {
                // just spin...
                async_yield();
            }
        }
    }

    await_signal_off(f.allocating);
    await(Close);
}
async_end

}
