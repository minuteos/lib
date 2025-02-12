/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/alloc_trace.h
 *
 * Allocation tracing support
 */

#pragma once

#include <base/base.h>

#if ALLOC_TRACE_ENABLE

struct __alloc_node
{
    __alloc_node* next;
    __alloc_node* prev;
    size_t size;
    const void* origin;
};

// space that must be available and freely writable before the pointer passed to __trace_alloc/free
#define ALLOC_TRACE_OVERHEAD    sizeof(__alloc_node)

EXTERN_C void ___trace_alloc(void* ptr, size_t size, const void* origin);
EXTERN_C void ___trace_free(void* ptr);

extern __alloc_node __alloc_trace;

#define __trace_alloc(ptr, size)    ___trace_alloc(ptr, size, __builtin_return_address(0))
#define __trace_free(ptr)           ___trace_free(ptr)

#else

#define ALLOC_TRACE_OVERHEAD    0

#define __trace_alloc(...)
#define __trace_free(...)

#endif
