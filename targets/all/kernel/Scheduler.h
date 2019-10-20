/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/Scheduler.h
 *
 * Simple cooperative task scheduler
 */

#pragma once

#include <kernel/config.h>
#include <kernel/async.h>

#include <collections/LinkedList.h>

namespace kernel
{

class Task;
template<typename... Args> class TaskWithArgs;

using PreSleepDelegate = Delegate<bool, mono_t, mono_t>;

//! Simple cooperative task scheduler
class Scheduler
{
public:
    //! Adds a task to the scheduler
    Task& Add(AsyncDelegate<> fn);

    //! Adds a task with arguments to the scheduler
    template<typename... Args> ALWAYS_INLINE Task& Add(AsyncDelegate<Args...> fn, Args... args)
    {
        return Add(new(MemPoolAllocDynamic<TaskWithArgs<Args...>>()) TaskWithArgs<Args...>(fn, args...));
    }

    //! Adds a task represented by a static function to the scheduler
    ALWAYS_INLINE Task& Add(async_fptr_t function)
    {
        return Add(AsyncDelegate<>(&__CallStatic, (void*)function));
    }

    //! Syntactic helper for @ref Scheduler::Add(AsyncDelegate)
    template<typename T, typename... Args> ALWAYS_INLINE Task& Add(T& target, async_methodptr_t<T, Args...> method, Args... args)
    {
        return Add(GetDelegate(&target, method), args...);
    }

    //! Syntactic helper for @ref Scheduler::Add(AsyncDelegate)
    template<typename T, typename... Args> ALWAYS_INLINE Task& Add(T* target, async_methodptr_t<T, Args...> method, Args... args)
    {
        return Add(GetDelegate(target, method), args...);
    }

    //! Adds a pre-sleep callback to the scheduler
    ALWAYS_INLINE void AddPreSleepCallback(PreSleepDelegate delegate) { preSleep.Push(delegate); }
    //! Adds a pre-sleep callback represented by a function to the scheduler
    ALWAYS_INLINE void AddPreSleepCallback(PreSleepDelegate::fptr_t function, void* arg0 = NULL) { AddPreSleepCallback(GetDelegate(function, arg0)); }
    //! Syntactic helper for @ref Scheduler::AddPreSleepCallback(AsyncDelegate)
    template<typename T> ALWAYS_INLINE void AddPreSleepCallback(T& target, PreSleepDelegate::mthptr_t<T> method) { AddPreSleepCallback(GetDelegate(&target, method)); }
    //! Syntactic helper for @ref Scheduler::AddPreSleepCallback(AsyncDelegate)
    template<typename T> ALWAYS_INLINE void AddPreSleepCallback(T* target, PreSleepDelegate::mthptr_t<T> method) { AddPreSleepCallback(GetDelegate(target, method)); }

    //! Removes a pre-sleep callback from the scheduler
    ALWAYS_INLINE void RemovePreSleepCallback(PreSleepDelegate delegate) { preSleep.Remove(delegate); }
    //! Removes a pre-sleep callback represented by a function from the scheduler
    ALWAYS_INLINE void RemovePreSleepCallback(PreSleepDelegate::fptr_t function, void* arg0 = NULL) { RemovePreSleepCallback(GetDelegate(function, arg0)); }
    //! Syntactic helper for @ref Scheduler::RemovePreSleepCallback(AsyncDelegate)
    template<typename T> ALWAYS_INLINE void RemovePreSleepCallback(T& target, PreSleepDelegate::mthptr_t<T> method) { RemovePreSleepCallback(GetDelegate(&target, method)); }
    //! Syntactic helper for @ref Scheduler::RemovePreSleepCallback(AsyncDelegate)
    template<typename T> ALWAYS_INLINE void RemovePreSleepCallback(T* target, PreSleepDelegate::mthptr_t<T> method) { RemovePreSleepCallback(GetDelegate(target, method)); }

    //! Executes the scheduled tasks
    mono_t Run();

    //! Retrieves current monotonic time used by the scheduler
    static ALWAYS_INLINE mono_t CurrentTime() { return MONO_CLOCKS; }
    //! Retrieves the main scheduler instance
    static ALWAYS_INLINE Scheduler& Main() { return s_main; }
    //! Retrieves the currently active scheduler instnace
    static ALWAYS_INLINE Scheduler& Current() { return *s_current; }

    //! Retrieves the currently running task
    ALWAYS_INLINE class Task& CurrentTask() { return *current; }

private:
    //! Adds a task to the scheduler
    Task& Add(Task* task);

    class Task* active = NULL;      //!< Queue of running tasks
    class Task* delayed = NULL;     //!< Queue of unconditionally sleeping tasks
    class Task* waiting = NULL;     //!< Queue of tasks waiting for a value to change
    class Task* current = NULL;     //!< Currently running task
    LinkedList<PreSleepDelegate> preSleep;  //!< List of callbacks called before sleep

    static Scheduler s_main;        //!< Main scheduler instance
    static Scheduler* s_current;     //!< Currently active scheduler

    friend struct ::AsyncFrame;
    friend class Task;

public:
    //! Wrapper for static functions to match the delegate signature
    static async_res_t __CallStatic(void* fptr, AsyncFrame** pCallee);
};

}
