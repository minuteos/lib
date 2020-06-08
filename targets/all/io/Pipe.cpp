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
    avail = exam = 0;
    woff = 1;
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
    size_t count;
    f.timeout = timeout.MakeAbsolute();

    while (f.written < data.Length())
    {
        count = await(WriterAllocate, 1, data.Length() - f.written, f.timeout);
        if (count == 0)
        {
            break;
        }

        memcpy(WriterBuffer().Pointer(), data.Pointer() + f.written, count);
        f.written += count;
        WriterAdvance(count);
    }

    async_return(f.written);
}
async_end

async(Pipe::WriterAllocate, size_t min, size_t req, Timeout timeout)
async_def()
{
    ASSERT(!IsClosed());

    if (wseg && woff < wseg->length && wseg->length - woff >= min)
    {
        MYTRACE("W: using pre-allocated space in current segment @ %p + %u", wseg->data + woff, std::min(wseg->length - woff, req));
        async_return(std::min(wseg->length - woff, req));
    }

    //need to allocate a new segment
    MYTRACE("W: allocating new segment (%u-%u)", min, req);
    PipeSegment* seg;
    seg = (PipeSegment*)await(allocator.AllocateSegment, min, req, timeout);
    if (!seg)
    {
        MYTRACE("W: could not allocate new segment");
        async_return(0);
    }

    MYTRACE("W: allocated %u byte segment %p", seg->length, seg);

    if (wseg)
    {
        // trim the current segment to the actually written length
        wseg->length = woff;
        wseg->next = seg;
    }
    else
    {
        // if this is the first segment being written, it also must be the first one being read
        ASSERT(!rseg);
        rseg = seg;
    }

    wseg = seg;
    woff = 0;
    async_return(std::min(wseg->length, req));
}
async_end

void Pipe::WriterAdvance(size_t count)
{
    ASSERT(wseg);
    ASSERT(woff + count <= wseg->length);
    MYTRACE("W: %u bytes written", count);
    woff += count;
    avail += count;
    total += count;
}

async(Pipe::ReaderRead, Timeout timeout)
async_def(
    Timeout timeout;
)
{
    f.timeout = timeout.MakeAbsolute();

    while (avail == exam && !IsClosed())
    {
        MYTRACE("R: waiting for data...");
        // wait for more data to become available
        if (!await_mask_not_timeout(total, ~0u, total, f.timeout))
        {
            break;
        }
    }

    MYTRACE("R: %u bytes available", avail);
    async_return(avail);
}
async_end

async(Pipe::ReaderReadUntil, uint8_t b, Timeout timeout)
async_def(
    Timeout timeout;
    PipeSegment* seg;
    size_t off;
)
{
    f.timeout = timeout.MakeAbsolute();

    if (!rseg)
    {
        // read initial data
        if (!await(ReaderRead, f.timeout))
        {
            async_return(0);
        }
    }

    f.seg = rseg;
    f.off = roff + exam;

    for (;;)
    {
        size_t remain;
        remain = avail - exam;
        if (!remain)
        {
            // need more data
            await(ReaderRead, f.timeout);
            if (!(remain = (avail - exam)))
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
                exam += p - (f.seg->data + f.off) + 1;
                async_return(exam);
            }
            else
            {
                // move to next segment
                exam += len;
                if ((remain -= len))
                {
                    f.off -= f.seg->length;
                    f.seg = f.seg->next;
                    ASSERT(f.seg);
                }
            }
        }
    }
}
async_end

void Pipe::ReaderExamined(size_t count)
{
    ASSERT(exam + count <= avail);
    MYTRACE("R: %u bytes examined", count);
    exam += count;
}

void Pipe::ReaderAdvance(size_t count)
{
    if (!count)
        return;

    ASSERT(count <= avail);
    ASSERT(rseg);
    MYTRACE("R: %u bytes read", count);
    avail -= count;
    exam = exam >= count ? exam - count : 0;

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
    wseg = NULL;
}

int Pipe::ReaderPeek(size_t offset) const
{
    for (auto seg = rseg; seg; seg = seg->next)
    {
        if (offset < seg->length)
            return seg->data[offset];
        offset -= seg->length;
    }
    return -1;
}

}
