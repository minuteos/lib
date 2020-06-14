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

    async(Read, size_t count = 1, Timeout timeout = Timeout::Infinite) { ASSERT(pipe); return async_forward(pipe->ReaderRead, count, timeout); }
    async(ReadUntil, uint8_t b, Timeout timeout = Timeout::Infinite) { ASSERT(pipe); return async_forward(pipe->ReaderReadUntil, b, timeout); }
    Span GetSpan(size_t offset = 0) const { ASSERT(pipe); return pipe->ReaderSpan(offset); }
    void Advance(size_t count) { ASSERT(pipe); pipe->ReaderAdvance(count); }
    void AdvanceTo(PipePosition position) { ASSERT(pipe); pipe->ReaderAdvanceTo(position); }

    PipePosition Position() const { ASSERT(pipe); return pipe->ReaderPosition(); }
    size_t Available() const { ASSERT(pipe); return pipe->ReaderAvailable(); }
    bool IsComplete() const { ASSERT(pipe); return pipe->ReaderComplete(); }
    int Peek(size_t offset) const { ASSERT(pipe); return pipe->ReaderPeek(offset); }
    size_t LengthUntil(PipePosition position) const { ASSERT(pipe); return pipe->ReaderLengthUntil(position); }

    bool Matches(Span data, size_t offset = 0) const { ASSERT(pipe); return pipe->ReaderMatches(data, offset); }

    Pipe::Iterator begin() const { ASSERT(pipe); return pipe->ReaderIteratorBegin(); }
    Pipe::Iterator end() const { ASSERT(pipe); return pipe->ReaderIteratorEnd(); }

private:
    Pipe* pipe;
};

}
