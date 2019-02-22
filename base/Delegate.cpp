/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * Delegate.cpp
 *
 * Implementation of pointer to member function decoding with virtual table lookup
 */

#include <base/base.h>

#include <base/Delegate.h>

Packed<_PMFDecodeResult> _PMFDecode(void* target, ptrdiff_t fptrOrVtOffset, ptrdiff_t thisAdjust)
{
    PackedWrapper<_PMFDecodeResult> res;
#if PMF_ODD_FPTR
    bool virt = thisAdjust & 1;
    thisAdjust >>= 1;
#else
    bool virt = fptrOrVtOffset & 1;
    fptrOrVtOffset &= ~1;
#endif

    // adjust the target first, so that it points to the correct vtable
    res.value.target = target = (void*)(uintptr_t(target) + thisAdjust);

    if (virt)
    {
        // find the actual method in the vtable
        res.value.fptr = *(_PMFDecodeResult::fptr_t*)(*(uintptr_t*)target + fptrOrVtOffset);
    }
    else
    {
        // direct pointer to function
        res.value.fptr = (_PMFDecodeResult::fptr_t)fptrOrVtOffset;
    }

    return res.packed;
};
