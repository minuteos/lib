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
    bool IsClosed() const { return !pwseg; }
    bool IsEmpty() const { return rpos == wpos; }
    bool IsCompleted() const { return IsEmpty() && IsClosed(); }
    void BindSignal(bool* sig) { wsignal = sig; }
    async(Completed, Timeout timeout = Timeout::Infinite);
    void Reset();
    size_t ThrottleLevel() const { return throttle; }
    void ThrottleLevel(size_t bytes) { throttle = bytes; }

    class Iterator
    {
    public:
        ALWAYS_INLINE constexpr bool operator ==(const Iterator& other) const { return remaining == other.remaining; }
        ALWAYS_INLINE constexpr bool operator !=(const Iterator& other) const { return remaining != other.remaining; }
        ALWAYS_INLINE constexpr Iterator& operator ++()
        {
            if (--remaining)
            {
                if (!++segRemaining)
                {
                    seg = seg->next;
                    segRemaining = -seg->length;
                    segEnd = seg->data - segRemaining;
                }
            }
            return *this;
        }
        ALWAYS_INLINE constexpr char operator *() const { return segEnd[segRemaining]; }
        ALWAYS_INLINE constexpr ptrdiff_t operator -(const Iterator& other) const { return other.remaining - remaining; }

        ALWAYS_INLINE constexpr Iterator begin() const { return *this; }
        ALWAYS_INLINE constexpr Iterator end() const { return Iterator(); }

    private:
        constexpr Iterator()
            : seg(NULL), segEnd(NULL), segRemaining(0), remaining(0) {}
        constexpr Iterator(PipeSegment* seg, size_t offset, size_t remaining)
            : seg(seg), segEnd(seg->data + seg->length), segRemaining(offset - seg->length), remaining(remaining) {}

        PipeSegment* seg;
        const uint8_t* segEnd;
        int segRemaining;
        size_t remaining;

        friend class Pipe;
    };

private:
    PipeAllocator& allocator;       //!< Allocator used for new segments
    PipeSegment* rseg = NULL;       //!< Pointer to the current read segment (head)
    size_t roff = 0;                //!< Offset into the current read segment
    PipeSegment** pwseg = &rseg;    //!< Pointer to pointer to current write segment (typically next member of last written segment)
    size_t woff = 0;                //!< Offset into the first write segment
    PipePosition rpos = 0;          //!< Read position
    PipePosition wpos = 0;          //!< Write position
    PipePosition apos = 0;          //!< Maximum allocated position
    size_t state = 0;               //!< Incremented every time pipe state changes
    bool* wsignal = NULL;           //!< External signal activated when new data is written to the pipe
    size_t throttle = 1024;         //!< Hold writes above this threshold

    void Cleanup();

    PipePosition WriterPosition() const { return wpos; }
    PipePosition WriterAllocatedPosition() const { return apos; }
    size_t WriterAvailable() const { return apos - wpos; }
    size_t WriterAvailableAfter(PipePosition position) const { ASSERT(position >= wpos); return position.LengthUntil(apos); }
    bool WriterCanAllocate() const { return !throttle || TotalBytes() < throttle; }
    bool WriterCanWrite() const { return WriterAvailable() || WriterCanAllocate(); }
    async(WriterAllocate, size_t block, Timeout timeout);
    async(WriterWrite, const char* data, size_t length, Timeout timeout);
    async(WriterWriteFV, Timeout timeout, const char* format, va_list va);
    RES_PAIR_DECL_EX(WriterBuffer, const, size_t offset);
    Buffer WriterBufferAt(PipePosition position) { return WriterBuffer(wpos.LengthUntil(position)); }
    void WriterAdvance(size_t count);
    void WriterAdvanceTo(PipePosition position) { if (auto count = wpos.LengthUntil(position)) WriterAdvance(count); }
    void WriterInsert(PipeSegment* seg);
    void WriterClose();
    void WriterSignal() { if (wsignal) { *wsignal = true; } }

    PipePosition ReaderPosition() const { return rpos; }
    size_t ReaderAvailable() const { return wpos - rpos; }
    bool ReaderAvailableFullSegment() const { return rseg && ReaderAvailable() >= (rseg->length - roff); }
    async(ReaderRequire, size_t count, Timeout timeout);
    async(ReaderRequireUntil, uint8_t b, Timeout timeout);
    async(ReaderRead, char* buffer, size_t length, Timeout timeout);
    RES_PAIR_DECL_EX(ReaderSpan, const, size_t offset);
    Span ReaderSpanAt(PipePosition position) const { return ReaderSpan(rpos.LengthUntil(position)); }
    RES_PAIR_DECL(ReaderRead, char* buffer, size_t length);
    void ReaderAdvance(size_t count) { ReaderRead(NULL, count); }
    void ReaderAdvanceTo(PipePosition position) { if (auto count = rpos.LengthUntil(position)) ReaderAdvance(count); }
    bool ReaderComplete() const { return IsClosed(); }
    int ReaderPeek(size_t offset) const;
    size_t ReaderLengthUntil(PipePosition position) const { return rpos.LengthUntil(position); }
    bool ReaderMatches(Span data, size_t offset) const;

    constexpr size_t TotalBytes() const { return apos - rpos; }

    constexpr Iterator ReaderIteratorBegin() const { return Iterator(rseg, roff, wpos - rpos); }
    constexpr Iterator ReaderIteratorBegin(size_t length) const { return Iterator(rseg, roff, std::min(length, size_t(wpos - rpos))); }
    constexpr Iterator ReaderIteratorEnd() const { return Iterator(); }

    static RES_PAIR_DECL(GetSpan, PipeSegment* seg, size_t offset, size_t count);

    static async(Copy, Pipe& from, Pipe& to, size_t offset, size_t length, Timeout timeout);
    static async(Move, Pipe& from, Pipe& to, size_t length, Timeout timeout);

    friend class PipeWriter;
    friend class PipeReader;
};

}
