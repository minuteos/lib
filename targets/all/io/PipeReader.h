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
class PipeWriter;

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

    async(Require, size_t count = 1, Timeout timeout = Timeout::Infinite) { ASSERT(pipe); return async_forward(pipe->ReaderRequire, count, timeout); }
    async(RequireUntil, uint8_t b, Timeout timeout = Timeout::Infinite) { ASSERT(pipe); return async_forward(pipe->ReaderRequireUntil, b, timeout); }
    async(Read, Buffer buffer, Timeout timeout = Timeout::Infinite) { ASSERT(pipe); return async_forward(pipe->ReaderRead, buffer.Pointer(), buffer.Length(), timeout); }
    async(CopyTo, io::PipeWriter writer, size_t offset, size_t count, Timeout timeout = Timeout::Infinite);
    async(MoveTo, io::PipeWriter writer, size_t count, Timeout timeout = Timeout::Infinite);
    Span GetSpan(size_t offset = 0) const { ASSERT(pipe); return pipe->ReaderSpan(offset); }
    Span Read(Buffer buffer) { ASSERT(pipe); return pipe->ReaderRead(buffer.Pointer(), buffer.Length()); }
    void Advance(size_t count) { ASSERT(pipe); pipe->ReaderAdvance(count); }
    void AdvanceTo(PipePosition position) { ASSERT(pipe); pipe->ReaderAdvanceTo(position); }

    PipePosition Position() const { ASSERT(pipe); return pipe->ReaderPosition(); }
    size_t Available() const { ASSERT(pipe); return pipe->ReaderAvailable(); }
    bool IsComplete() const { ASSERT(pipe); return pipe->ReaderComplete(); }
    int Peek(size_t offset) const { ASSERT(pipe); return pipe->ReaderPeek(offset); }
    size_t LengthUntil(PipePosition position) const { ASSERT(pipe); return pipe->ReaderLengthUntil(position); }

    bool Matches(Span data, size_t offset = 0) const { ASSERT(pipe); return pipe->ReaderMatches(data, offset); }

    ALWAYS_INLINE constexpr Pipe::Iterator begin() const { ASSERT(pipe); return pipe->ReaderIteratorBegin(); }
    ALWAYS_INLINE constexpr Pipe::Iterator end() const { ASSERT(pipe); return pipe->ReaderIteratorEnd(); }

    ALWAYS_INLINE constexpr Pipe::Iterator Enumerate(size_t length) const { ASSERT(pipe); return pipe->ReaderIteratorBegin(length); }

private:
    Pipe* pipe;

    friend class PipeWriter;
};

}

#include <io/PipeWriter.h>

inline async(io::PipeReader::CopyTo, io::PipeWriter writer, size_t offset, size_t length, Timeout timeout)
{
    ASSERT(pipe);
    ASSERT(writer.pipe);
    return async_forward(Pipe::Copy, *pipe, *writer.pipe, offset, length, timeout);
}

inline async(io::PipeReader::MoveTo, io::PipeWriter writer, size_t length, Timeout timeout)
{
    ASSERT(pipe);
    ASSERT(writer.pipe);
    return async_forward(Pipe::Move, *pipe, *writer.pipe, length, timeout);
}
