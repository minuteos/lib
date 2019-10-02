/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/async.cpp
 *
 * General support for asynchronous functions
 */

#include <kernel/async.h>

//! Prolog allocation of a frame from a memory pool
NO_INLINE async_res_t _async_prolog_pool(AsyncFrame** pCallee, const AsyncSpec* spec)
{
    AsyncFrame* f = *pCallee = (AsyncFrame*)spec->pool->Alloc();
    f->spec = spec;
    return _ASYNC_RES(f, spec->start);
}

//! Prolog allocation of an oversized frame dynamically using @ref malloc
NO_INLINE async_res_t _async_prolog_dynamic(AsyncFrame** pCallee, const AsyncSpec* spec)
{
    AsyncFrame* f = *pCallee = (AsyncFrame*)malloc(spec->frameSize);
    memset(f, 0, spec->frameSize);
    f->spec = spec;
    return _ASYNC_RES(f, spec->start);
}

/*!
 * Called every time an asynchronous function is entered,
 * allocates and initializes a new frame on first entry
 *
 * Returns a tuple containing the called frame pointer and
 * continuation address
 */
NO_INLINE async_res_t _async_prolog(AsyncFrame** pCallee, const AsyncSpec* spec)
{
    if (auto f = *pCallee)
        return _ASYNC_RES(f, f->cont);
    else if (spec->pool)
        return _async_prolog_pool(pCallee, spec);
    else
        return _async_prolog_dynamic(pCallee, spec);
}

/*!
 * Called once an asynchronous function finishes executing.
 * Deallocates function frame and returns the final value (optimized for easy tail-chaining)
 */
NO_INLINE async_res_t _async_epilog(AsyncFrame** pCallee, intptr_t result)
{
    auto callee = *pCallee;
    *pCallee = NULL;
    if (callee->spec->pool)
        callee->spec->pool->Free(callee);
    else
        free(callee);

    return _ASYNC_RES(result, AsyncResult::Complete);
}