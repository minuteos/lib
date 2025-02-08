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

    PipePosition Position() const { ASSERT(pipe); return pipe->WriterPosition(); }
    PipePosition AllocatedPosition() const { ASSERT(pipe); return pipe->WriterAllocatedPosition(); }
    size_t Available() const { ASSERT(pipe); return pipe->WriterAvailable(); }
    size_t AvailableAfter(PipePosition pos) const { ASSERT(pipe); return pipe->WriterAvailableAfter(pos); }
    bool CanAllocate() const { ASSERT(pipe); return pipe->WriterCanAllocate(); }
    //! Allocates a new block, throwing an error on timeout or if the pipe is closed
    async(Allocate, size_t block, Timeout timeout = Timeout::Infinite) { ASSERT(pipe); return async_forward(pipe->WriterAllocate, block, timeout); }
    //! Writes data to the pipe, throwing an error unless data can be written in full
    async(Write, Span data, Timeout timeout = Timeout::Infinite) { ASSERT(pipe); return async_forward(pipe->WriterWrite, data.Pointer(), data.Length(), timeout); }
    //! Writes a formatted string to the pipe, throwing an error unless it can be written in full
    async(WriteF, const char* format, ...) async_def_va(WriteFV, format, Timeout::Infinite, format);
    //! Writes a formatted string to the pipe, throwing an error unless it can be written in full within the specified timeout
    async(WriteFTimeout, Timeout timeout, const char* format, ...) async_def_va(WriteFV, format, timeout, format);
    //! Writes a formatted string to the pipe, throwing an error unless it can be written in full within the specified timeout
    async(WriteFV, Timeout timeout, const char* format, va_list va) { ASSERT(pipe); return async_forward(pipe->WriterWriteFV, timeout, format, va); }
    async(Empty, Timeout timeout = Timeout::Infinite) { ASSERT(pipe); return async_forward(pipe->Empty, timeout); }
    async_once(Change, Timeout timeout = Timeout::Infinite) { ASSERT(pipe); return async_forward(pipe->Change, timeout); }
    Buffer GetBuffer(size_t offset = 0) { ASSERT(pipe); return pipe->WriterBuffer(offset); }
    Buffer GetBufferAt(PipePosition position) { ASSERT(pipe); return pipe->WriterBufferAt(position); }
    void Advance(size_t count) { ASSERT(pipe); pipe->WriterAdvance(count); }
    void Close() { ASSERT(pipe); pipe->WriterClose(); }
    bool IsClosed() const { ASSERT(pipe); return pipe->IsClosed(); }

    ALWAYS_INLINE Pipe::Iterator EnumerateFrom(PipePosition start) const { ASSERT(pipe); return pipe->ReaderIteratorBegin() + size_t(start - pipe->ReaderPosition()); }

    constexpr operator bool() const { return pipe; }

private:
    Pipe* pipe;

    friend class PipeReader;
};

}
