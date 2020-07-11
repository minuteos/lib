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

#include <utility>

namespace kernel
{

class Task;
template<typename... Args> class TaskWithArgs;
template<typename... Args> class TaskFnWithArgs;

using PreSleepDelegate = Delegate<bool, mono_t, mono_t>;

//! Simple cooperative task scheduler
class Scheduler
{
public:
    //! Adds a task to the scheduler
    Task& Add(AsyncDelegate<> fn);

    //! Adds a task with arguments to the scheduler
    template<typename... Args, typename... AArgs> ALWAYS_INLINE Task& Add(AsyncDelegate<Args...> fn, AArgs&&... args)
    {
        return Add(new(MemPoolAllocDynamic<TaskWithArgs<Args...>>()) TaskWithArgs<Args...>(fn, std::forward<AArgs>(args)...));
    }

    //! Adds a task represented by a static function to the scheduler
    ALWAYS_INLINE Task& Add(async_fptr_t function)
    {
        return Add(AsyncDelegate<>(&__CallStatic, (void*)function));
    }

    //! Adds a task represented by a static function with arguments to the scheduler
    template<typename... Args, typename... AArgs> ALWAYS_INLINE Task& Add(async_fptr_args_t<Args...> function, AArgs&&... args)
    {
        return Add(new(MemPoolAllocDynamic<TaskFnWithArgs<Args...>>()) TaskFnWithArgs<Args...>(function, std::forward<AArgs>(args)...));
    }

    //! Syntactic helper for @ref Scheduler::Add(AsyncDelegate)
    template<typename T, typename... Args, typename... AArgs> ALWAYS_INLINE Task& Add(T& target, async_methodptr_t<T, Args...> method, AArgs&&... args)
    {
        return Add(GetDelegate(&target, method), std::forward<AArgs>(args)...);
    }

    //! Syntactic helper for @ref Scheduler::Add(AsyncDelegate)
    template<typename T, typename... Args, typename... AArgs> ALWAYS_INLINE Task& Add(T* target, async_methodptr_t<T, Args...> method, AArgs&&... args)
    {
        return Add(GetDelegate(target, method), std::forward<AArgs>(args)...);
    }

#if KERNEL_SYNC_ONLY
    //! Adds a pre-sleep callback to the scheduler
    ALWAYS_INLINE void AddPreSleepCallback(PreSleepDelegate delegate) { }
    //! Removes a pre-sleep callback from the scheduler
    ALWAYS_INLINE void RemovePreSleepCallback(PreSleepDelegate delegate) { }
#else
    //! Adds a pre-sleep callback to the scheduler
    ALWAYS_INLINE void AddPreSleepCallback(PreSleepDelegate delegate) { preSleep.Push(delegate); }
    //! Removes a pre-sleep callback from the scheduler
    ALWAYS_INLINE void RemovePreSleepCallback(PreSleepDelegate delegate) { preSleep.Remove(delegate); }
#endif

    //! Adds a pre-sleep callback represented by a function to the scheduler
    ALWAYS_INLINE void AddPreSleepCallback(PreSleepDelegate::fptr_t function, void* arg0 = NULL) { AddPreSleepCallback(GetDelegate(function, arg0)); }
    //! Syntactic helper for @ref Scheduler::AddPreSleepCallback(AsyncDelegate)
    template<typename T> ALWAYS_INLINE void AddPreSleepCallback(T& target, PreSleepDelegate::mthptr_t<T> method) { AddPreSleepCallback(GetDelegate(&target, method)); }
    //! Syntactic helper for @ref Scheduler::AddPreSleepCallback(AsyncDelegate)
    template<typename T> ALWAYS_INLINE void AddPreSleepCallback(T* target, PreSleepDelegate::mthptr_t<T> method) { AddPreSleepCallback(GetDelegate(target, method)); }

    //! Removes a pre-sleep callback represented by a function from the scheduler
    ALWAYS_INLINE void RemovePreSleepCallback(PreSleepDelegate::fptr_t function, void* arg0 = NULL) { RemovePreSleepCallback(GetDelegate(function, arg0)); }
    //! Syntactic helper for @ref Scheduler::RemovePreSleepCallback(AsyncDelegate)
    template<typename T> ALWAYS_INLINE void RemovePreSleepCallback(T& target, PreSleepDelegate::mthptr_t<T> method) { RemovePreSleepCallback(GetDelegate(&target, method)); }
    //! Syntactic helper for @ref Scheduler::RemovePreSleepCallback(AsyncDelegate)
    template<typename T> ALWAYS_INLINE void RemovePreSleepCallback(T* target, PreSleepDelegate::mthptr_t<T> method) { RemovePreSleepCallback(GetDelegate(target, method)); }

    //! Executes the scheduled tasks
    mono_t Run();

#if !KERNEL_SYNC_ONLY
    //! Retrieves current monotonic time used by the scheduler
    static ALWAYS_INLINE mono_t CurrentTime() { return MONO_CLOCKS; }
#endif
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
#if !KERNEL_SYNC_ONLY
    class Task* delayed = NULL;     //!< Queue of unconditionally sleeping tasks
#endif
    class Task* waiting = NULL;     //!< Queue of tasks waiting for a value to change
    class Task** nextWaiting = &waiting;    //!< Insertion pointer for the next waiting task
    class Task* current = NULL;     //!< Currently running task
#if !KERNEL_SYNC_ONLY
    LinkedList<PreSleepDelegate> preSleep;  //!< List of callbacks called before sleep
#endif

    static Scheduler s_main;        //!< Main scheduler instance
    static Scheduler* s_current;     //!< Currently active scheduler

    friend struct ::AsyncFrame;
    friend class Task;

public:
    //! Wrapper for static functions to match the delegate signature
    static async_res_t __CallStatic(void* fptr, AsyncFrame** pCallee);
};

}
