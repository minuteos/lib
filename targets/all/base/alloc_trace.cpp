/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/alloc_trace.cpp
 */

#include "alloc_trace.h"

#if ALLOC_TRACE_ENABLE

#define MYDBG(...)  DBGCL("AT", __VA_ARGS__)

//#define ALLOC_TRACE_TRACE   1

#if ALLOC_TRACE_TRACE
#define MYTRACE(...)    MYDBG(__VA_ARGS__)
#else
#define MYTRACE(...)
#endif

__alloc_node __alloc_trace = { &__alloc_trace, &__alloc_trace };

void insert_after(__alloc_node& t, __alloc_node& i)
{
    i.next = t.next;
    i.prev = &t;
    t.next = t.next->prev = &i;
}

void remove(__alloc_node& n)
{
    n.prev->next = n.next;
    n.next->prev = n.prev;
    n.prev = n.next = NULL;
}

void ___trace_alloc(void* ptr, size_t size, const void* origin)
{
    ASSERT(ptr);
    MYTRACE("alloc: %p", ptr);
    auto node = (__alloc_node*)ptr - 1;
    node->size = size;
    node->origin = origin;
    insert_after(__alloc_trace, *node);
}

void ___trace_free(void* ptr)
{
    MYTRACE("free: %p", ptr);
    auto node = (__alloc_node*)ptr - 1;
    remove(*node);
}

#endif
