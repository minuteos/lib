/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/Events.cpp
 */

#include "Events.h"

namespace kernel
{

_EventTable* _EventTable::active;

void _EventTable::AddHandler(_Delegate handler)
{
    // register the table as active
    if (!next && active != this)
    {
        next = active;
        active = this;
    }

    auto newHandler = (_EventHandler*)MemPoolAlloc<_EventHandler>();
    newHandler->next = handlers;
    newHandler->handler = handler;
    handlers = newHandler;
}

void _EventTable::RemoveHandler(_Delegate d)
{
    _EventHandler** pNext = &handlers;
    for (_EventHandler* p = *pNext; p != NULL; p = *pNext)
    {
        if (p->handler == d)
        {
            *pNext = p->next;
            MemPoolFree(p);
        }
        else
        {
            pNext = &p->next;
        }
    }
}

void _EventTable::RemoveHandlers(const void* owner)
{
    _EventHandler** pNext = &handlers;
    for (_EventHandler* p = *pNext; p != NULL; p = *pNext)
    {
        if (_DelegateTarget(p->handler) == owner)
        {
            *pNext = p->next;
            MemPoolFree(p);
        }
        else
        {
            pNext = &p->next;
        }
    }
}

void UnregisterEvents(const void* owner)
{
    for (_EventTable* tbl = _EventTable::active; tbl; tbl = tbl->next)
    {
        tbl->RemoveHandlers(owner);
    }
}

OPTIMIZE bool _EventTable::FireEvent(const void* event, bool* handled)
{
    auto h = handlers;

    if (!h)
    {
        return false;
    }

    bool wasHandled;
    if (!handled)
    {
        handled = &wasHandled;
    }
    *handled = false;

    while (h)
    {
        h->delegate(event, handled);
        if (*handled)
        {
            return true;
        }
        h = h->next;
    }

    return false;
}

}
