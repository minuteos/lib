/*
 * Copyright (c) 2020 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/PipeSegment.cpp
 */

#include "PipeSegment.h"

namespace io
{

bool PipeSegment::Matches(size_t offset, Span data) const
{
    union MultiPtr
    {
        const void* p;
        const uint8_t* b;
        const uint32_t* w;
    };

    auto seg = this;
    MultiPtr dataPtr = { data.Pointer() };
    MultiPtr dataEnd = { data.end() };

    while (offset >= seg->length)
    {
        offset -= seg->length;
        seg = seg->next;
        ASSERT(seg);
    }

    MultiPtr segPtr = { seg->data + offset };
    MultiPtr segEnd = { seg->data + seg->length };

    for (;;)
    {
        while (segPtr.w + 1 <= segEnd.w && dataPtr.w + 1 <= dataEnd.w)
        {
            if (*segPtr.w++ != *dataPtr.w++)
            {
                return false;
            }
        }

        while (segPtr.b < segEnd.b && dataPtr.b < dataEnd.b)
        {
            if (*segPtr.b++ != *dataPtr.b++)
            {
                return false;
            }
        }

        if (dataPtr.p == dataEnd.p)
        {
            return true;
        }

        seg = seg->next;
        ASSERT(seg);
        segPtr.p = seg->data;
        segEnd.p = seg->data + seg->length;
    }
}

}
