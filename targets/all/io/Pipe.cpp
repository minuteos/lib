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

//#define PIPE_TRACE  1

#if PIPE_TRACE
#define MYTRACE(fmt, ...)        DBGCL("pipe", "[%p] " fmt, this, ## __VA_ARGS__)
#else
#define MYTRACE(...)
#endif

namespace io
{

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
    rseg = wseg = NULL;
    roff = 0;
    rpos = apos = wpos;
    woff = 1;
}

void Pipe::Reset()
{
    MYTRACE("Reset");
    if (rseg)
    {
        Cleanup();
    }
    rseg = wseg = NULL;
    rpos = apos = wpos = 0;
    roff = woff = total = 0;
}

async(Pipe::Completed, Timeout timeout)
async_def(
    Timeout timeout
)
{
    f.timeout = timeout.MakeAbsolute();

    while (!IsCompleted())
    {
        if (!await_mask_not_timeout(total, ~0u, total, f.timeout))
        {
            break;
        }
    }

    async_return(IsCompleted());
}
async_end

async(Pipe::WriterWrite, Span data, Timeout timeout)
async_def(
    Timeout timeout;
    size_t written
)
{
    f.timeout = timeout.MakeAbsolute();

    while (data.Length() > f.written)
    {
        if (wpos == apos && !await(WriterAllocate, data.Length() - f.written, f.timeout))
        {
            async_return(-f.written);
        }

        ASSERT(wseg);
        ASSERT(wseg->length > woff);
        size_t count = std::min(data.Length() - f.written, wseg->length - woff);
        memcpy((uint8_t*)wseg->data + woff, data.Pointer() + f.written, count);
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
        f.seg = wseg;
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

async(Pipe::WriterAllocate, size_t hint, Timeout timeout)
async_def()
{
    if (IsClosed())
    {
        MYTRACE("W: no allocation on closed pipe");
        async_return(0);
    }

    MYTRACE("W: allocating new segment (hint: %u)", hint);
    PipeSegment* seg;
    seg = (PipeSegment*)await(allocator.AllocateSegment, hint, timeout);
    if (!seg)
    {
        MYTRACE("W: could not allocate new segment");
        async_return(0);
    }

    MYTRACE("W: allocated %u byte segment %p", seg->length, seg);

    if (wseg)
    {
        // append the new segment after the last segment
        auto* last = wseg;
        while (last->next) last = last->next;
        last->next = seg;
        if (woff == wseg->length)
        {
            // make the allocated space available immediately
            ASSERT(wseg->next == seg);
            wseg = wseg->next;
            woff = 0;
        }
    }
    else
    {
        // if this is the first segment being written, initialize the reader as well
        ASSERT(!rseg);
        rseg = seg;
        wseg = seg;
        woff = 0;
    }

    apos += seg->length;
    async_return(seg->length);
}
async_end

void Pipe::WriterAdvance(size_t count)
{
    ASSERT(wseg);
    ASSERT(wpos + count <= apos);
    MYTRACE("W: %u bytes written", count);
    woff += count;
    wpos += count;
    total += count;
    while (woff > wseg->length)
    {
        woff -= wseg->length;
        wseg = wseg->next;
        ASSERT(wseg);
    }
    if (woff == wseg->length && wseg->next)
    {
        wseg = wseg->next;
        woff = 0;
    }
}

void Pipe::WriterClose()
{
    MYTRACE("W: pipe closed @ %u", wpos);
    wseg = NULL;
    woff = 1;
    total++;

    if (IsEmpty())
    {
        MYTRACE("W: pipe completed, early cleanup");
        Cleanup();
    }
}

async(Pipe::ReaderRead, size_t count, Timeout timeout)
async_def(
    Timeout timeout;
)
{
    f.timeout = timeout.MakeAbsolute();

    while (rpos + count > wpos && !IsClosed())
    {
        MYTRACE("R: waiting for data...");
        // wait for more data to become available
        if (!await_mask_not_timeout(total, ~0u, total, f.timeout))
        {
            break;
        }
    }

    MYTRACE("R: %u bytes available", wpos - rpos);
    async_return(wpos - rpos);
}
async_end

async(Pipe::ReaderReadUntil, uint8_t b, Timeout timeout)
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
        if (!await(ReaderRead, 1, f.timeout))
        {
            async_return(0);
        }
    }

    f.seg = rseg;
    f.off = roff;
    f.epos = rpos;

    for (;;)
    {
        size_t remain = wpos - f.epos;
        if (!remain)
        {
            // need more data
            await(ReaderRead, f.epos - rpos + 1, f.timeout);
            if (!(remain = (wpos - f.epos)))
            {
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
            if (auto p = (const uint8_t*)memchr(f.seg->data + f.off, b, len))
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

void Pipe::ReaderAdvance(size_t count)
{
    if (!count)
        return;

    ASSERT(rpos + count <= wpos);
    ASSERT(rseg);
    MYTRACE("R: %u bytes read", count);
    rpos += count;

    if (IsCompleted())
    {
        // early release of all segments
        MYTRACE("R: pipe completed, early cleanup");
        Cleanup();
        return;
    }

    size_t remain = rseg->length - roff;
    if (remain > count)
    {
        // more left in the current segment
        roff += count;
        MYTRACE("R: %u bytes remaining in current segment", rseg->length - roff);
        return;
    }

    // release segments that are no longer needed
    count -= remain;
    auto last = rseg;

    for (;;)
    {
        auto next = last->next;
        last->Release();
        MYTRACE("R: released segment %p", last);

        if (!next)
        {
            // all segments released
            break;
        }

        last = next;
        if (count < last->length)
        {
            // part of the segment remains
            rseg = last;
            roff = count;
            MYTRACE("R: continuing in segment %p offset %u", rseg, roff);
            return;
        }

        count -= last->length;
    }

    // all segments have been released, release write segment as well
    ASSERT(!count);
    ASSERT(wseg == last);
    MYTRACE("R: all segments released");
    rseg = wseg = NULL;
    roff = woff = 0;
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

bool Pipe::ReaderMatches(Span data, size_t offset) const
{
    if (rpos + offset + data.Length() > wpos)
    {
        return false;
    }

    if (!data.Length())
    {
        return true;
    }

    offset += roff;
    auto seg = rseg;
    while (offset >= seg->length)
    {
        offset -= seg->length;
        seg = seg->next;
        ASSERT(seg);
    }

    do
    {
        auto len = std::min(data.Length(), seg->length - offset);
        if (memcmp(seg->data + offset, data.Pointer(), len))
        {
            return false;
        }
        data = data.RemoveLeft(len);
    } while (data);

    return true;
}

res_pair_t Pipe::ReaderSpan(size_t offset) const
{
    ASSERT(rpos + offset <= wpos);
    return GetSpan(rseg, roff + offset, wpos - rpos - offset);
}

res_pair_t Pipe::WriterBuffer(size_t offset) const
{
    ASSERT(wpos + offset <= apos);
    return GetSpan(wseg, woff + offset, ~0u);
}

res_pair_t Pipe::GetSpan(PipeSegment* seg, size_t offset, size_t count)
{
    for (;;)
    {
        ASSERT(seg);
        if (seg->length > offset)
            return Span(seg->data + offset, std::min(count, seg->length - offset));
        offset -= seg->length;
        seg = seg->next;
    }
}

}
