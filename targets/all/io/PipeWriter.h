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

    async(Allocate, size_t min, size_t req, mono_t waitUntil = 0) { ASSERT(pipe); return async_forward(pipe->WriterAllocate, min, req, waitUntil); }
    async(AllocateExact, size_t count, mono_t waitUntil = 0) { return async_forward(Allocate, count, count, waitUntil); }
    async(AllocateUpTo, size_t count, mono_t waitUntil = 0) { return async_forward(Allocate, 1, count, waitUntil); }
    async(Write, Span data, mono_t waitUntil = 0) { ASSERT(pipe); return async_forward(pipe->WriterWrite, data, waitUntil); }
    void Enqueue(PipeSegment* seg) { ASSERT(pipe); pipe->WriterEnqueue(seg); }
    void Advance(size_t count) { ASSERT(pipe); pipe->WriterAdvance(count); }
    void Close() { ASSERT(pipe); pipe->WriterClose(); }

private:
    Pipe* pipe;
};

}
