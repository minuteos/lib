/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/PipeWriter.h
 */

#pragma once

#include <kernel/kernel.h>

#include <io/Pipe.h>

namespace io
{

class DuplexPipe;

class PipeWriter
{

public:
    constexpr PipeWriter()
        : pipe(NULL) {}
    constexpr PipeWriter(Pipe& pipe)
        : pipe(&pipe) {}
    constexpr PipeWriter(const PipeWriter& writer)
        : pipe(writer.pipe) {}
    constexpr PipeWriter(const DuplexPipe& pipe);

    async(Allocate, size_t min, size_t req, Timeout timeout = Timeout::Infinite) { ASSERT(pipe); return async_forward(pipe->WriterAllocate, min, req, timeout); }
    async(AllocateExact, size_t count, Timeout timeout = Timeout::Infinite) { return async_forward(Allocate, count, count, timeout); }
    async(AllocateUpTo, size_t count, Timeout timeout = Timeout::Infinite) { return async_forward(Allocate, 1, count, timeout); }
    async(Write, Span data, Timeout timeout = Timeout::Infinite) { ASSERT(pipe); return async_forward(pipe->WriterWrite, data, timeout); }
    void Enqueue(PipeSegment* seg) { ASSERT(pipe); pipe->WriterEnqueue(seg); }
    void Advance(size_t count) { ASSERT(pipe); pipe->WriterAdvance(count); }
    size_t Write(Span data) { ASSERT(pipe); return pipe->WriterWrite(data); }
    void Close() { ASSERT(pipe); pipe->WriterClose(); }

private:
    Pipe* pipe;
};

}
