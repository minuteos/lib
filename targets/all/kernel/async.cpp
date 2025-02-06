/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/async.cpp
 *
 * General support for asynchronous functions
 */

#include <kernel/kernel.h>

//! Prolog allocation of a frame from a memory pool
NO_INLINE async_prolog_t _async_prolog_pool(AsyncFrame** pCallee, const AsyncSpec* spec)
{
    AsyncFrame* f = *pCallee = (AsyncFrame*)spec->pool->Alloc();
    f->spec = spec;
    return pack<_async_prolog_t>(f, spec->start);
}

//! Prolog allocation of an oversized frame dynamically using @ref malloc
NO_INLINE async_prolog_t _async_prolog_dynamic(AsyncFrame** pCallee, const AsyncSpec* spec)
{
    AsyncFrame* f = *pCallee = (AsyncFrame*)malloc(spec->frameSize);
    memset(f, 0, spec->frameSize);
    f->spec = spec;
    return pack<_async_prolog_t>(f, spec->start);
}

/*!
 * Called every time an asynchronous function is entered,
 * allocates and initializes a new frame on first entry
 *
 * Returns a tuple containing the called frame pointer and
 * continuation address
 */
NO_INLINE async_prolog_t _async_prolog(AsyncFrame** pCallee, const AsyncSpec* spec)
{
    if (auto f = *pCallee)
        return pack<_async_prolog_t>(f, f->cont);
    else if (spec->pool)
        return _async_prolog_pool(pCallee, spec);
    else
#if !KERNEL_SYNC_ONLY
        return _async_prolog_dynamic(pCallee, spec);
#else
        return async_res_t();
#endif
}

/*!
 * Called once an asynchronous function finishes executing.
 * Deallocates function frame and returns the final value (optimized for easy tail-chaining)
 */
NO_INLINE async_res_t _async_epilog(async_res_t res, AsyncFrame** pCallee)
{
    auto callee = *pCallee;
    *pCallee = NULL;
    if (callee->spec->pool)
        callee->spec->pool->Free(callee);
#if !KERNEL_SYNC_ONLY
    else
        free(callee);
#endif

    return res;
}

NO_INLINE async_res_t AsyncFrame::_prepare_wait(AsyncResult type)
{
    bool match = !(*(uint8_t*)waitPtr);
    if (match != (type && AsyncResult::_WaitInvertedMask))
    {
        type = AsyncResult::Complete;
        waitPtr = NULL;
        waitResult = true;
    }

    return _ASYNC_RES(this, type);
}

NO_INLINE async_res_t AsyncFrame::_prepare_wait(AsyncResult type, uintptr_t mask, uintptr_t expect)
{
    expect &= mask;
    bool match = (*waitPtr & mask) == expect;
    if (match != (type && AsyncResult::_WaitInvertedMask))
    {
        // matched
        if (type && AsyncResult::_WaitAcquireMask)
        {
            *waitPtr ^= mask;
        }
        waitPtr = NULL;
        waitResult = true;
        return _ASYNC_RES(this, AsyncResult::Complete);
    }

    kernel::Scheduler::s_current->current->wait.mask = mask;
    kernel::Scheduler::s_current->current->wait.expect = expect;
    return _ASYNC_RES(this, type);
}

void AsyncFrame::_child_completed(intptr_t res)
{
    children--;
}
