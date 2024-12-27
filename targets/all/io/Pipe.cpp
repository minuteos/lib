/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/Pipe.cpp
 *
 * Simple zero-copy pipe implementation
 * inspired by the .NET System.IO.Pipelines
 */

#include <io/Pipe.h>

#include <base/format.h>
#include <base/MemPoolAsync.h>

//#define PIPE_TRACE  1

#if PIPE_TRACE
#define MYTRACEX(...)            DBGCL("pipe", __VA_ARGS__)
#define MYTRACE(fmt, ...)        DBGCL("pipe", "[%p] " fmt, this, ## __VA_ARGS__)
#else
#define MYTRACEX(...)
#define MYTRACE(...)
#endif

namespace io
{

struct PipeReferencedSegment : PipeSegment
{
    PipeReferencedSegment(PipeSegment* inner, const uint8_t* data, size_t length)
        : PipeSegment(data, length), inner(inner)
    {
        inner->Reference();
    }

    virtual void Destroy()
    {
        inner->Release();
        MemPoolFree<PipeReferencedSegment>(this);
    }

    PipeSegment* inner;
};

void Pipe::Cleanup()
{
    MYTRACE("Cleanup");

    auto seg = rseg;
    while (seg)
    {
        auto next = seg->next;
        seg->Release();
        MYTRACE("R: released segment %p", seg);
        seg = next;
    }
    rseg = NULL;
    pwseg = NULL;
    roff = woff = 0;
    rpos = apos = wpos;
}

void Pipe::Reset()
{
    MYTRACE("Reset");
    if (rseg)
    {
        Cleanup();
    }
    rseg = NULL;
    pwseg = &rseg;
    rpos = apos = wpos = 0;
    roff = woff = 0;
    state++;
    WriterSignal();
}

async(Pipe::Completed, Timeout timeout)
async_def(
    Timeout timeout
)
{
    f.timeout = timeout.MakeAbsolute();

    while (!IsCompleted())
    {
        if (!await_mask_not_timeout(state, ~0u, state, f.timeout))
        {
            break;
        }
    }

    async_return(IsCompleted());
}
async_end


async(Pipe::Empty, Timeout timeout)
async_def(
    Timeout timeout
)
{
    f.timeout = timeout.MakeAbsolute();

    while (!IsEmpty())
    {
        if (!await(Change, f.timeout))
        {
            break;
        }
    }

    async_return(IsEmpty());
}
async_end

async(Pipe::WriterWrite, const char* data, size_t length, Timeout timeout)
async_def(
    Timeout timeout;
    size_t written
)
{
    f.timeout = timeout.MakeAbsolute();

    while (f.written < length)
    {
        if (wpos == apos && !await(WriterAllocate, length - f.written, f.timeout))
        {
            async_return(-f.written);
        }

        ASSERT(pwseg && *pwseg);
        ASSERT((*pwseg)->length > woff);
        size_t count = std::min(length - f.written, (*pwseg)->length - woff);
        memcpy((uint8_t*)(*pwseg)->data + woff, data + f.written, count);
        f.written += count;
        WriterAdvance(count);
    }

    async_return(f.written);
}
async_end

async(Pipe::WriterWriteFV, Timeout timeout, const char* format, va_list va)
async_def(
    Timeout timeout;
    size_t written, length, offset;
    PipeSegment* seg;

    static void format_output(void* context, char ch) { ((__FRAME*)context)->Output(ch); }
    void Output(char ch)
    {
        if (++length > written && seg)
        {
            written++;
            ((char*)seg->data)[offset++] = ch;
            if (offset >= seg->length)
            {
                seg = seg->next;
                offset = 0;
            }
        }
    }
)
{
    f.timeout = timeout.MakeAbsolute();

    do
    {
        if (wpos == apos && !await(WriterAllocate, f.length - f.written, f.timeout))
        {
            async_return(-f.written);
        }

        auto before = f.written;
        f.length = 0;
        f.seg = *pwseg;
        f.offset = woff;
        va_list va2;
        va_copy(va2, va);
        vformat(&f.format_output, &f, format, va2);
        va_end(va2);
        WriterAdvance(f.written - before);
    } while (f.written < f.length);

    async_return(f.written);
}
async_end

void Pipe::WriterInsert(PipeSegment* seg)
{
    if (woff)
    {
        // cut current write segment, either discarding the remainder or re-linking if it makes sense
        if (woff < (*pwseg)->length)
        {
            size_t remaining = (*pwseg)->length - woff;
            void* splitMem;
            if (remaining <= sizeof(PipeReferencedSegment) || !(splitMem = MemPoolAlloc<PipeReferencedSegment>()))
            {
                MYTRACE("W: discarding last %d bytes from current write segment", remaining);
                apos -= remaining;
            }
            else
            {
                MYTRACE("W: splitting off last %d bytes from current write segment", remaining);
                auto seg = new(splitMem) PipeReferencedSegment(*pwseg, (*pwseg)->data + woff, remaining);
                (*pwseg)->next = seg;
            }
            (*pwseg)->length = woff;
        }

        pwseg = &(*pwseg)->next;
        woff = 0;
    }

    ASSERT(!seg->next);
    seg->next = *pwseg;
    *pwseg = seg;
    pwseg = &seg->next;
    MYTRACE("W: inserted %d byte segment %p @ %d", seg->length, seg, wpos);
    wpos += seg->length;
    apos += seg->length;
    state++;
    WriterSignal();
}

async(Pipe::Copy, Pipe& from, Pipe& to, size_t offset, size_t length, Timeout timeout)
async_def(
    Timeout timeout;
    size_t written;
    PipeSegment* seg;
    size_t offset;
)
{
    f.timeout = timeout.MakeAbsolute();

    ASSERT(from.ReaderAvailable() >= offset + length);

    f.seg = from.rseg;
    f.offset = from.roff + offset;

    while (f.offset >= f.seg->length)
    {
        f.offset -= f.seg->length;
        f.seg = f.seg->next;
        ASSERT(f.seg);
    }

    while (f.written < length)
    {
        ASSERT(f.seg);
        while (!to.WriterCanAllocate())
        {
            MYTRACEX("[%p] > [%p] COPY: throttling at %d bytes", &from, &to, to.TotalBytes());
            if (!await_mask_not_timeout(to.state, ~0u, to.state, f.timeout) || to.IsClosed())
            {
                MYTRACEX("[%p] > [%p] COPY: stopping at %d bytes", &from, &to, to.TotalBytes());
                async_return(0);
            }
        }

        void* mem;
        if ((mem = MemPoolAlloc<PipeReferencedSegment>()))
        {
            // reference part of source segment in the destination pipe
            auto seg = new(mem) PipeReferencedSegment(f.seg, f.seg->data + f.offset, std::min(length - f.written, f.seg->length - f.offset));
            MYTRACEX("[%p] > [%p] COPY: referenced bytes %d+%d from segment %p", &from, &to, f.offset, seg->length, f.seg);
            f.offset += seg->length;
            if (f.offset >= f.seg->length)
            {
                f.offset -= f.seg->length;
                f.seg = f.seg->next;
            }
            to.WriterInsert(seg);
            f.written += seg->length;
        }
        else
        {
            if (!await_mempool_timeout(PipeReferencedSegment, f.timeout))
            {
                break;
            }
        }
    }

    async_return(f.written);
}
async_end

async(Pipe::Move, Pipe& from, Pipe& to, size_t length, Timeout timeout)
async_def(
    Timeout timeout;
    size_t written;
)
{
    f.timeout = timeout.MakeAbsolute();

    ASSERT(from.ReaderAvailable() >= length);

    while (f.written < length)
    {
        while (!to.WriterCanAllocate())
        {
            MYTRACEX("[%p] > [%p] MOVE: throttling at %d bytes", &from, &to, to.TotalBytes());
            if (!await_mask_not_timeout(to.state, ~0u, to.state, f.timeout) || to.IsClosed())
            {
                MYTRACEX("[%p] > [%p] MOVE: stopping at %d bytes", &from, &to, to.TotalBytes());
                async_return(0);
            }
        }

        ASSERT(from.rseg);

        PipeSegment* seg;
        void* mem;
        if (!from.roff && from.rseg->length < length - f.written)
        {
            // transfer an entire segment from source to destination
            seg = from.rseg;
            seg->Reference();
            MYTRACEX("[%p] > [%p] MOVE: transfered entire %d byte segment %p", &from, &to, from.rseg->length, from.rseg);
        }
        else if ((mem = MemPoolAlloc<PipeReferencedSegment>()))
        {
            // reference part of source segment in the destination pipe
            seg = new(mem) PipeReferencedSegment(from.rseg, from.rseg->data + from.roff, std::min(length - f.written, from.rseg->length - from.roff));
            MYTRACEX("[%p] > [%p] MOVE: referenced bytes %d+%d from segment %p", &from, &to, from.roff, seg->length, from.rseg);
        }
        else
        {
            if (!await_mempool_timeout(PipeReferencedSegment, f.timeout))
            {
                break;
            }
            continue;
        }

        from.ReaderAdvance(seg->length);
        ASSERT(from.rseg != seg);
        to.WriterInsert(seg);
        f.written += seg->length;
    }

    async_return(f.written);
}
async_end

async(Pipe::WriterAllocate, size_t hint, Timeout timeout)
async_def(
    Timeout timeout;
)
{
    if (IsClosed())
    {
        MYTRACE("W: no allocation on closed pipe");
        async_return(0);
    }

    f.timeout = timeout;

    while (!WriterCanAllocate())
    {
        MYTRACE("W: throttling at %d bytes", TotalBytes());
        if (!await_mask_not_timeout(state, ~0u, state, f.timeout))
        {
            MYTRACE("W: could not allocate new segment, %d bytes in pipe", TotalBytes());
            async_return(0);
        }
        if (IsClosed())
        {
            MYTRACE("W: pipe closed while waiting for allocation");
            async_return(0);
        }
    }

    MYTRACE("W: allocating new segment (hint: %u)", hint);
    PipeSegment* seg;
    seg = (PipeSegment*)await(allocator.AllocateSegment, hint, f.timeout);
    if (!seg)
    {
        MYTRACE("W: could not allocate new segment");
        async_return(0);
    }

    MYTRACE("W: allocated %u byte segment %p", seg->length, seg);

    if (auto* last = *pwseg)
    {
        // append the new segment after the last segment
        while (last->next) last = last->next;
        last->next = seg;
    }
    else
    {
        // if this is the first segment being written, it must point to rseg
        ASSERT(rseg || pwseg == &rseg);
        *pwseg = seg;
        woff = 0;
    }

    apos += seg->length;
    state++;

    async_return(seg->length);
}
async_end

void Pipe::WriterAdvance(size_t count)
{
    ASSERT(*pwseg);
    ASSERT(wpos + count <= apos);
    MYTRACE("W: %u bytes written", count);
    woff += count;
    wpos += count;
    state++;
    WriterSignal();
    while (woff >= (*pwseg)->length)
    {
        woff -= (*pwseg)->length;
        pwseg = &(*pwseg)->next;
    }
}

void Pipe::WriterClose()
{
    MYTRACE("W: pipe closed @ %u", wpos);
    pwseg = NULL;
    woff = 0;
    state++;
    WriterSignal();

    if (IsEmpty())
    {
        MYTRACE("W: pipe completed, early cleanup");
        Cleanup();
    }
}

async(Pipe::ReaderRequire, size_t count, Timeout timeout)
async_def(
    Timeout timeout;
)
{
    f.timeout = timeout.MakeAbsolute();

    while (rpos + count > wpos && !IsClosed())
    {
        MYTRACE("R: waiting for data...");
        // wait for more data to become available
        if (!await_mask_not_timeout(state, ~0u, state, f.timeout))
        {
            break;
        }
    }

    MYTRACE("R: %u bytes available", wpos - rpos);
    async_return(wpos - rpos);
}
async_end

async(Pipe::ReaderRequireUntil, uint8_t b, Timeout timeout)
async_def(
    Timeout timeout;
    PipeSegment* seg;
    size_t off;
    PipePosition epos;
)
{
    f.timeout = timeout.MakeAbsolute();

    if (!rseg)
    {
        // read initial data
        if (!await(ReaderRequire, 1, f.timeout))
        {
            MYTRACE("RRU: no data");
            async_return(0);
        }
    }

    f.seg = rseg;
    f.off = roff;
    f.epos = rpos;

    for (;;)
    {
        size_t remain;
        if (!(remain = (wpos - f.epos)))
        {
            // need more data
            await(ReaderRequire, f.epos - rpos + 1, f.timeout);
            if (!(remain = (wpos - f.epos)))
            {
                MYTRACE("RRU: no data");
                async_return(0);
            }
        }

        // move to the current segment
        while (f.off >= f.seg->length)
        {
            f.off -= f.seg->length;
            f.seg = f.seg->next;
            ASSERT(f.seg);
        }

        // process available data
        while (remain)
        {
            size_t len = std::min(remain, f.seg->length - f.off);
            const uint8_t* p;
            if ((p = (const uint8_t*)memchr(f.seg->data + f.off, b, len)))
            {
                // found a match
                f.epos += p - (f.seg->data + f.off) + 1;
                async_return(f.epos - rpos);
            }
            else
            {
                // move to next segment
                f.epos += len;
                if ((remain -= len))
                {
                    f.off = 0;
                    f.seg = f.seg->next;
                    ASSERT(f.seg);
                }
                else
                {
                    f.off += len;
                }
            }
        }
    }
}
async_end

async(Pipe::ReaderRead, char* data, size_t length, Timeout timeout)
async_def(
    Timeout timeout;
    size_t read;
)
{
    f.timeout = timeout.MakeAbsolute();

    while (f.read < length)
    {
        size_t avail;
        if (!(avail = ReaderAvailable()))
        {
            await(ReaderRequire, 1, f.timeout);
            if (!(avail = ReaderAvailable()))
                break;
        }
        f.read += Span(ReaderRead(data + f.read, std::min(length - f.read, avail))).Length();
    }

    async_return(f.read);
}
async_end

Span::packed_t Pipe::ReaderRead(char* buffer, size_t count)
{
    if (!count)
        return Span(buffer, size_t(0));

    ASSERT(rpos + count <= wpos);
    ASSERT(rseg);
    MYTRACE("R: %u bytes read", count);
    rpos += count;
    state++;

    auto bufferStart = buffer;
    size_t remain = rseg->length - roff;
    if (remain > count)
    {
        // more left in the current segment
        if (buffer)
        {
            memcpy(buffer, rseg->data + roff, count);
            buffer += count;
        }
        roff += count;
        MYTRACE("R: %u bytes remaining in current segment", rseg->length - roff);
    }
    else
    {
        // release segments that are no longer needed
        if (buffer)
        {
            memcpy(buffer, rseg->data + roff, remain);
            buffer += remain;
        }
        count -= remain;
        auto last = rseg;

        for (;;)
        {
            auto next = last->next;
            if (pwseg == &last->next)
            {
                pwseg = &rseg;
            }
            last->next = NULL;
            last->Release();
            MYTRACE("R: released segment %p", last);

            if (!next)
            {
                // all segments have been released
                ASSERT(!count);
                ASSERT(!pwseg || pwseg == &rseg);
                MYTRACE("R: all segments released");
                rseg = NULL;
                roff = woff = 0;
                break;
            }

            last = next;
            if (count < last->length)
            {
                // part of the segment remains
                if (buffer)
                {
                    memcpy(buffer, last->data, count);
                    buffer += count;
                }
                rseg = last;
                roff = count;
                MYTRACE("R: continuing in segment %p offset %u", rseg, roff);
                break;
            }
            else if (buffer)
            {
                memcpy(buffer, last->data, last->length);
                buffer += last->length;
            }

            count -= last->length;
        }
    }

    return Span(bufferStart, buffer);
}

int Pipe::ReaderPeek(size_t offset) const
{
    offset += roff;
    for (auto seg = rseg; seg; seg = seg->next)
    {
        if (offset < seg->length)
            return seg->data[offset];
        offset -= seg->length;
    }
    return -1;
}

Span::packed_t Pipe::ReaderPeek(char* buf, size_t length, size_t offset) const
{
    offset += roff;
    char* start = buf;
    for (auto seg = rseg; length && seg; seg = seg->next)
    {
        if (offset < seg->length)
        {
            size_t block = std::min(seg->length - offset, length);
            memcpy(buf, seg->data + offset, block);
            length -= block;
            buf += block;
            offset = 0;
        }
        else
        {
            offset -= seg->length;
        }
    }
    return Span(start, buf);
}

bool Pipe::ReaderMatches(Span data, size_t offset) const
{
    return ReaderIteratorBegin().Matches(data, offset);
}

Span::packed_t Pipe::ReaderSpan(size_t offset) const
{
    ASSERT(rpos + offset <= wpos);
    return GetSpan(rseg, roff + offset, wpos - rpos - offset);
}

Buffer::packed_t Pipe::WriterBuffer(size_t offset) const
{
    ASSERT(wpos + offset <= apos);
    return (Buffer::packed_t)GetSpan(*pwseg, woff + offset, ~0u);
}

Span::packed_t Pipe::GetSpan(PipeSegment* seg, size_t offset, size_t count)
{
    for (;;)
    {
        if (!seg)
        {
            return {};
        }
        if (seg->length > offset)
        {
            return Span(seg->data + offset, std::min(count, seg->length - offset));
        }
        offset -= seg->length;
        seg = seg->next;
    }
}

void Pipe::Iterator::Skip(size_t length)
{
    if (length > remaining)
    {
        length = remaining;
    }
    remaining -= length;

    while (length >= size_t(-segRemaining))
    {
        seg = seg->next;
        length += segRemaining;
        if (!seg)
        {
            ASSERT(!length);
            ASSERT(!remaining);
            segRemaining = 0;
        }
        else
        {
            segRemaining = -seg->length;
            segEnd = seg->data - segRemaining;
        }
    }

    segRemaining += length;
}

Buffer::packed_t Pipe::Iterator::ReadImpl(char* buffer, size_t length)
{
    if (length > remaining)
    {
        length = remaining;
    }
    Buffer res(buffer, length);
    remaining -= length;

    while (length >= size_t(-segRemaining))
    {
        memcpy(buffer, segEnd + segRemaining, -segRemaining);
        buffer -= segRemaining;
        length += segRemaining;
        seg = seg->next;
        ASSERT(seg);
        segRemaining = -seg->length;
        segEnd = seg->data - segRemaining;
    }

    memcpy(buffer, segEnd + segRemaining, length);
    segRemaining += length;
    return res;
}

}
