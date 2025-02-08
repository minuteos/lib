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
    //! Waits for the pipe to complete (become empty and closed), returns false on timeout
    async(Completed, Timeout timeout = Timeout::Infinite);
    //! Waits for the pipe to become empty, returns false on timeout
    async(Empty, Timeout timeout = Timeout::Infinite);
    //! Waits for the next change in pipe state, returns false on timeout
    async_once(Change, Timeout timeout = Timeout::Infinite) { return async_forward(WaitMaskNot, state, ~0u, state, timeout); }
    void Reset();
    size_t ThrottleLevel() const { return throttle; }
    void ThrottleLevel(size_t bytes) { throttle = bytes; }

    class SpanIterator
    {
    public:
        SpanIterator() = default;

        ALWAYS_INLINE constexpr bool operator ==(const SpanIterator& other) const { return remaining == other.remaining; }
        ALWAYS_INLINE constexpr bool operator !=(const SpanIterator& other) const { return remaining != other.remaining; }
        ALWAYS_INLINE constexpr SpanIterator& operator ++()
        {
            if (remaining > seg->length - offset)
            {
                remaining -= seg->length - offset;
                seg = seg->next;
                ASSERT(seg);
                offset = 0;
            }
            else
            {
                remaining = 0;
            }

            return *this;
        }
        ALWAYS_INLINE constexpr Span operator *() const { return Span(seg->data + offset, std::min(remaining, seg->length - offset)); }

        ALWAYS_INLINE constexpr SpanIterator begin() const { return *this; }
        ALWAYS_INLINE constexpr SpanIterator end() const { return SpanIterator(); }

        ALWAYS_INLINE constexpr operator bool() const { return !!remaining; }
        ALWAYS_INLINE constexpr size_t Available() const { return remaining; }

    private:
        constexpr SpanIterator(PipeSegment* seg, size_t offset, size_t remaining)
            : seg(seg), offset(offset), remaining(remaining) {}

        PipeSegment* seg;
        size_t offset, remaining;

        friend class Pipe;
    };

    class Iterator
    {
    public:
        Iterator() = default;

        ALWAYS_INLINE constexpr bool operator ==(const Iterator& other) const { return remaining == other.remaining; }
        ALWAYS_INLINE constexpr bool operator !=(const Iterator& other) const { return remaining != other.remaining; }
        ALWAYS_INLINE constexpr Iterator& operator ++()
        {
            if (--remaining)
            {
                if (!++segRemaining)
                {
                    seg = seg->next;
                    ASSERT(seg);
                    segRemaining = -seg->length;
                    segEnd = seg->data - segRemaining;
                }
            }
            return *this;
        }
        ALWAYS_INLINE constexpr char operator *() const { return segEnd[segRemaining]; }
        ALWAYS_INLINE constexpr ptrdiff_t operator -(const Iterator& other) const { return other.remaining - remaining; }
        ALWAYS_INLINE Iterator operator +(size_t offset) const { auto res = *this; res.Skip(offset); return res; }

        ALWAYS_INLINE constexpr Iterator begin() const { return *this; }
        ALWAYS_INLINE constexpr Iterator end() const { return Iterator(); }

        ALWAYS_INLINE constexpr operator bool() const { return !!remaining; }
        ALWAYS_INLINE constexpr size_t Available() const { return remaining; }

        void Skip(size_t length);
        Buffer Read(Buffer buf) { return ReadImpl(buf.Pointer(), buf.Length()); }
        bool Consume(char ch) { return *this && **this == ch ? (++*this, true) : false; }
        int Read(int def = -1) { if (*this) { def = **this; ++*this; }; return def; }
        ALWAYS_INLINE bool Matches(Span data, size_t offset = 0) const { return remaining >= offset + data.Length() && seg->Matches(segEnd + segRemaining - seg->data + offset, data); }

        ALWAYS_INLINE constexpr SpanIterator Spans() const { return SpanIterator(seg, seg->length + segRemaining, remaining); }

    private:
        constexpr Iterator(PipeSegment* seg, size_t offset, size_t remaining)
            : seg(seg), segEnd(seg->data + seg->length), segRemaining(offset - seg->length), remaining(remaining) {}

        PipeSegment* seg;
        const uint8_t* segEnd;
        int segRemaining;
        size_t remaining;

        Buffer::packed_t ReadImpl(char* data, size_t length);

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
    //! Allocates a new block, throwing an error on timeout or if the pipe is closed
    async(WriterAllocate, size_t block, Timeout timeout);
    //! Writes data to the pipe, throwing an error unless the data is written in full
    async(WriterWrite, const char* data, size_t length, Timeout timeout);
    //! Writes formatted data to the pipe, throwing an error unless the data is written in full
    async(WriterWriteFV, Timeout timeout, const char* format, va_list va);
    Buffer::packed_t WriterBuffer(size_t offset) const;
    Buffer WriterBufferAt(PipePosition position) { return WriterBuffer(wpos.LengthUntil(position)); }
    void WriterAdvance(size_t count);
    void WriterAdvanceTo(PipePosition position) { if (auto count = wpos.LengthUntil(position)) WriterAdvance(count); }
    void WriterInsert(PipeSegment* seg);
    void WriterClose();
    void WriterSignal() { if (wsignal) { *wsignal = true; } }

    PipePosition ReaderPosition() const { return rpos; }
    size_t ReaderAvailable() const { return wpos - rpos; }
    bool ReaderAvailableFullSegment() const { return rseg && ReaderAvailable() >= (rseg->length - roff); }
    //! Waits until the required number of bytes is available for reading, throwing an error on timeout
    //! Less than the request count (or 0) can be returned when the stream is closed
    async(ReaderRequire, size_t count, Timeout timeout);
    //! Waits until the specified byte appears in the stream, throwing an error on timeout
    async(ReaderRequireUntil, uint8_t b, Timeout timeout);
    //! Reads up to the specified number of bytes from the pipes
    async(ReaderRead, char* buffer, size_t length, Timeout timeout);
    Span::packed_t ReaderSpan(size_t offset) const;
    Span ReaderSpanAt(PipePosition position) const { return ReaderSpan(rpos.LengthUntil(position)); }
    Span::packed_t ReaderRead(char* buffer, size_t length);
    void ReaderAdvance(size_t count) { ReaderRead(NULL, count); }
    void ReaderAdvanceTo(PipePosition position) { if (auto count = rpos.LengthUntil(position)) ReaderAdvance(count); }
    bool ReaderComplete() const { return IsClosed(); }
    int ReaderPeek(size_t offset) const;
    Span::packed_t ReaderPeek(char * buffer, size_t length, size_t offset) const;
    size_t ReaderLengthUntil(PipePosition position) const { return rpos.LengthUntil(position); }
    bool ReaderMatches(Span data, size_t offset) const;

    constexpr size_t TotalBytes() const { return apos - rpos; }

    constexpr Iterator ReaderIteratorBegin() const { return Iterator(rseg, roff, wpos - rpos); }
    constexpr Iterator ReaderIteratorBegin(size_t length) const { return Iterator(rseg, roff, std::min(length, size_t(wpos - rpos))); }
    constexpr Iterator ReaderIteratorEnd() const { return Iterator(); }

    constexpr SpanIterator ReaderSpanIteratorBegin() const { return SpanIterator(rseg, roff, wpos - rpos); }
    constexpr SpanIterator ReaderSpanIteratorBegin(size_t length) const { return SpanIterator(rseg, roff, std::min(length, size_t(wpos - rpos))); }
    constexpr SpanIterator ReaderSpanIteratorEnd() const { return SpanIterator(); }

    static Span::packed_t GetSpan(PipeSegment* seg, size_t offset, size_t count);

    //! Copies (i.e. does not read) the specified number of bytes from the source pipe to the destination, reusing as much memory as possible
    //! The from pipe must already contain enough data, throws if the data cannot be written
    static async(Copy, Pipe& from, Pipe& to, size_t offset, size_t length, Timeout timeout);
    //! Moves (i.e. reads/removes) the specified number of bytes from the source pipe to the destination, reusing as much memory as possible
    //! The from pipe must already contain enough data, thorws if the data cannot be written
    static async(Move, Pipe& from, Pipe& to, size_t length, Timeout timeout);

    friend class PipeWriter;
    friend class PipeReader;
};

}
