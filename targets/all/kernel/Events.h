/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/Events.h
 *
 * A simple event handling library
 */

#pragma once

#include <base/base.h>
#include <base/Delegate.h>
#include <base/MemPool.h>

namespace kernel
{

class _EventHandler
{
    union { _Delegate handler; Delegate<void, const void*, bool*> delegate; };
    _EventHandler* next;

    friend class _EventTable;
};

template<typename TEvent> bool FireEvent(const TEvent& evt);
template<typename TEvent> void _RegisterEvent(_Delegate handler);
template<typename TEvent> void _UnregisterEvent(_Delegate handler);
void UnregisterEvents(const void* owner);

class _EventTable
{
    static _EventTable* active;
    _EventTable* next;
    _EventHandler* handlers;

protected:
    void AddHandler(_Delegate handler);
    void RemoveHandler(_Delegate handler);
    void RemoveHandlers(const void* owner);
    bool FireEvent(const void* evt, bool* handled);

    template<typename TEvent> friend bool ::kernel::FireEvent(const TEvent& evt);
    template<typename TEvent> friend void ::kernel::_RegisterEvent(_Delegate handler);
    template<typename TEvent> friend void ::kernel::_UnregisterEvent(_Delegate handler);
    friend void ::kernel::UnregisterEvents(const void* owner);
};

template<typename TEvent> ALWAYS_INLINE _EventTable& _GetEventTable()
{
	static _EventTable instance;
	return instance;
}

template<typename TEvent> ALWAYS_INLINE bool* _GetEventHandledField(const TEvent& evt) { return NULL; }

template<typename TEvent> ALWAYS_INLINE bool FireEvent(const TEvent& evt)
{
    return _GetEventTable<const TEvent>().FireEvent(&evt, _GetEventHandledField(evt));
}

template<typename TEvent> ALWAYS_INLINE void _RegisterEvent(_Delegate handler)
{
    _GetEventTable<const TEvent>().AddHandler(handler);
}

template<typename TEvent> ALWAYS_INLINE void RegisterEvent(Delegate<void, const TEvent&> handler)
{
    return _RegisterEvent<TEvent>(handler);
}

template<typename TEvent> ALWAYS_INLINE void RegisterEvent(Delegate<void, const TEvent&, bool&> handler)
{
    return _RegisterEvent<TEvent>(handler);
}

template<typename TEvent, class TOwner> ALWAYS_INLINE void RegisterEvent(TOwner* owner, void (TOwner::*handler)(const TEvent& evt))
{
    return RegisterEvent<TEvent>(Delegate(owner, handler));
}

template<typename TEvent, class TOwner> ALWAYS_INLINE void RegisterEvent(TOwner* owner, void (TOwner::*handler)(const TEvent& evt, bool& handled))
{
    return RegisterEvent<TEvent>(Delegate(owner, handler));
}

template<typename TEvent> ALWAYS_INLINE void _UnregisterEvent(_Delegate handler)
{
    _GetEventTable<const TEvent>().RemoveHandler(handler);
}

template<typename TEvent> ALWAYS_INLINE void UnregisterEvent(Delegate<void, const TEvent&> handler)
{
    return _UnregisterEvent<TEvent>(handler);
}

template<typename TEvent> ALWAYS_INLINE void UnregisterEvent(Delegate<void, const TEvent&, bool&> handler)
{
    return _UnregisterEvent<TEvent>(handler);
}

template<typename TEvent, class TOwner> ALWAYS_INLINE void UnregisterEvent(TOwner* owner, void (TOwner::*handler)(const TEvent& evt))
{
    return UnregisterEvent<TEvent>(Delegate(owner, handler));
}

template<typename TEvent, class TOwner> ALWAYS_INLINE void UnregisterEvent(TOwner* owner, void (TOwner::*handler)(const TEvent& evt, bool& handled))
{
    return UnregisterEvent<TEvent>(Delegate(owner, handler));
}

void UnregisterEvents(const void* owner);

class EventTarget
{
protected:
    template <class TEvent, class TOwner> void RegisterEvent(void (TOwner::*handler)(const TEvent& evt)) { ::kernel::RegisterEvent((TOwner*)this, handler); }
    template <class TEvent, class TOwner> void RegisterEvent(void (TOwner::*handler)(const TEvent& evt, bool& handled)) { ::kernel::RegisterEvent((TOwner*)this, handler); }

    template <class TEvent, class TOwner> void UnregisterEvent(void (TOwner::*handler)(const TEvent& evt)) { ::kernel::UnregisterEvent((TOwner*)this, handler); }
    template <class TEvent, class TOwner> void UnregisterEvent(void (TOwner::*handler)(const TEvent& evt, bool& handled)) { ::kernel::UnregisterEvent((TOwner*)this, handler); }
};

class DynamicEventTarget : public EventTarget
{
public:
    ~DynamicEventTarget() { UnregisterEvents(this); }
};

#define DEFINE_EVENT_HANDLED_FIELD(type, field) \
template<> ALWAYS_INLINE bool* _GetEventHandledField(const type& inst) { return (bool*)&inst.field; }

}
