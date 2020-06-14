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
#include <io/PipePosition.h>

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

    size_t Unprocessed() const { return wpos - rpos; }
    bool IsClosed() const { return !wseg && woff; }
    bool IsEmpty() const { return rpos == wpos; }
    bool IsCompleted() const { return IsEmpty() && IsClosed(); }
    async(Completed, Timeout timeout = Timeout::Infinite);
    void Reset();

    class Iterator
    {
    public:
        ALWAYS_INLINE constexpr bool operator ==(const Iterator& other) const { return remaining == other.remaining; }
        ALWAYS_INLINE constexpr bool operator !=(const Iterator& other) const { return remaining != other.remaining; }
        ALWAYS_INLINE constexpr Iterator& operator ++()
        {
            if (--remaining)
            {
                if (++offset >= seg->length)
                {
                    seg = seg->next;
                    offset = 0;
                }
            }
            return *this;
        }
        ALWAYS_INLINE constexpr char operator *() const { return seg->data[offset]; }
        ALWAYS_INLINE constexpr ptrdiff_t operator -(const Iterator& other) const { return other.remaining - remaining; }

    private:
        constexpr Iterator()
            : seg(NULL), offset(0), remaining(0) {}
        constexpr Iterator(PipeSegment* seg, size_t offset, size_t remaining)
            : seg(seg), offset(offset), remaining(remaining) {}

        PipeSegment* seg;
        size_t offset, remaining;

        friend class Pipe;
    };

private:
    PipeAllocator& allocator;       //!< Allocator used for new segments
    PipeSegment* rseg = NULL;       //!< Pointer to the current read segment (head)
    size_t roff = 0;                //!< Offset into the current read segment
    PipeSegment* wseg = NULL;       //!< Pointer to the first write segment
    size_t woff = 0;                //!< Offset into the first write segment
    PipePosition rpos = 0;          //!< Read position
    PipePosition wpos = 0;          //!< Write position
    PipePosition apos = 0;          //!< Maximum allocated position
    size_t total = 0;               //!< Number of bytes moved through the pipe in total, also incremented when closed

    void Cleanup();

    PipePosition WriterPosition() const { return wpos; }
    PipePosition WriterAllocatedPosition() const { return apos; }
    size_t WriterAvailable() const { return apos - wpos; }
    size_t WriterAvailableAfter(PipePosition position) const { ASSERT(position >= wpos); return position.LengthUntil(apos); }
    async(WriterAllocate, size_t block, Timeout timeout);
    async(WriterWrite, Span data, Timeout timeout);
    async(WriterWriteFV, Timeout timeout, const char* format, va_list va);
    RES_PAIR_DECL_EX(WriterBuffer, const, size_t offset);
    Buffer WriterBufferAt(PipePosition position) { return WriterBuffer(wpos.LengthUntil(position)); }
    void WriterAdvance(size_t count);
    void WriterAdvanceTo(PipePosition position) { if (auto count = wpos.LengthUntil(position)) WriterAdvance(count); }
    void WriterClose();

    PipePosition ReaderPosition() const { return rpos; }
    size_t ReaderAvailable() const { return wpos - rpos; }
    async(ReaderRead, size_t count, Timeout timeout);
    async(ReaderReadUntil, uint8_t b, Timeout timeout);
    RES_PAIR_DECL_EX(ReaderSpan, const, size_t offset);
    Span ReaderSpanAt(PipePosition position) const { return ReaderSpan(rpos.LengthUntil(position)); }
    void ReaderAdvance(size_t count);
    void ReaderAdvanceTo(PipePosition position) { if (auto count = rpos.LengthUntil(position)) ReaderAdvance(count); }
    bool ReaderComplete() const { return IsClosed(); }
    int ReaderPeek(size_t offset) const;
    size_t ReaderLengthUntil(PipePosition position) const { return rpos.LengthUntil(position); }
    bool ReaderMatches(Span data, size_t offset) const;

    Iterator ReaderIteratorBegin() { return Iterator(rseg, roff, wpos - rpos); }
    Iterator ReaderIteratorEnd() { return Iterator(); }

    static RES_PAIR_DECL(GetSpan, PipeSegment* seg, size_t offset, size_t count);

    friend class PipeWriter;
    friend class PipeReader;
};

}
