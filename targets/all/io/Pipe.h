/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/Pipe.h
 */

#pragma once

#include <kernel/kernel.h>

#include <base/Span.h>

#include <io/PipeSegment.h>
#include <io/PipeAllocator.h>

namespace io
{

class Pipe
{
public:
    Pipe(PipeAllocator& allocator = *PipeAllocator::s_default)
        : allocator(allocator)
    {
    }

    ~Pipe()
    {
        Cleanup();
    }

    size_t Unprocessed() const { return avail; }
    bool IsClosed() const { return !wseg && woff; }
    bool IsEmpty() const { return !avail; }
    bool IsCompleted() const { return !avail && IsClosed(); }
    async(Completed, mono_t until = 0);

private:
    PipeAllocator& allocator;       //!< Allocator used for new segments
    PipeSegment* wseg = NULL;       //!< Pointer to the current write segment (last segment)
    size_t woff = 0;                //!< Offset into the current write segment
    PipeSegment* rseg = NULL;       //!< Pointer to the current read segment (first segment)
    size_t roff = 0;                //!< Offset into the current read segment
    size_t avail = 0;               //!< Number of bytes available to read
    size_t exam = 0;                //!< Number of bytes already examined
    size_t total = 0;               //!< Number of bytes moved through the pipe in total, also incremented when closed

    void Cleanup();

    void WriterAdvance(size_t count);
    void WriterEnqueue(PipeSegment* segment);
    async(WriterAllocate, size_t min, size_t req, mono_t waitUntil);
    async(WriterWrite, Span data, mono_t waitUntil);
    Buffer WriterBuffer() { ASSERT(wseg && woff < wseg->length); return Buffer((uint8_t*)wseg->data + woff, wseg->length - woff); }
    void WriterClose() { wseg = NULL; woff = 1; total++; }

    async(ReaderRead, mono_t waitUntil);
    async(ReaderReadUntil, uint8_t b, mono_t waitUntil);
    void ReaderAdvance(size_t count);
    void ReaderExamined(size_t count);
    size_t ReaderAvailable() const { return avail; }
    Span ReaderFirstSpan() const { ASSERT(rseg && roff < rseg->length); return Span(rseg->data + roff, std::min(rseg->length - roff, avail)); }
    bool ReaderComplete() const { return IsClosed(); }
    int ReaderPeek(size_t offset) const;

    friend class PipeWriter;
    friend class PipeReader;
};

}
