/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/PipeReader.h
 */

#pragma once

#include <base/base.h>

#include <io/Pipe.h>

namespace io
{

class DuplexPipe;

class PipeReader
{
public:
    constexpr PipeReader()
        : pipe(NULL) {}
    constexpr PipeReader(Pipe& pipe)
        : pipe(&pipe) {}
    constexpr PipeReader(const PipeReader& reader)
        : pipe(reader.pipe) {}
    constexpr PipeReader(const DuplexPipe& pipe);

    async(Read, mono_t waitUntil = 0) { ASSERT(pipe); return async_forward(pipe->ReaderRead, waitUntil); }
    async(ReadUntil, uint8_t b, mono_t waitUntil = 0) { ASSERT(pipe); return async_forward(pipe->ReaderReadUntil, b, waitUntil); }
    void Examined(size_t count) { ASSERT(pipe); pipe->ReaderExamined(count); }
    void Advance(size_t count) { ASSERT(pipe); pipe->ReaderAdvance(count); }

    size_t Available() const { ASSERT(pipe); return pipe->ReaderAvailable(); }
    Span FirstSpan() const { ASSERT(pipe); return pipe->ReaderFirstSpan(); }
    bool IsComplete() const { ASSERT(pipe); return pipe->ReaderComplete(); }
    int Peek(size_t offset) const { ASSERT(pipe); return pipe->ReaderPeek(offset); }

private:
    Pipe* pipe;
};

}
