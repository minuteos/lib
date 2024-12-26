/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/Task.h
 */

#pragma once

#include <kernel/config.h>
#include <kernel/async.h>

#include <tuple>

namespace kernel
{

#if KERNEL_STATS

struct _TaskStats
{
    int ticks, cycles, maxCycles;
    int delays, delayChecks, delayEnds;
    int waits, waitChecks, waitEnds, waitTimeouts;
};

#endif

//! Representation of a single task
class Task
{
private:
    enum
    {
        MaxRunAll = 32,  //!< Maximum task count for RunAll (limit of @ref index field)
    };

    Task* next;         //!< Link to the next task in the queue
    AsyncFrame* top;    //!< Pointer to the topmost frame of the async stack
    AsyncDelegate<> fn; //!< Function implmenting the task
    Delegate<void, intptr_t> onComplete; //!< Delegate called on completion
#if KERNEL_STATS && KERNEL_STATS_PER_TASK
    _TaskStats stats;
#endif
    struct
    {
#if !KERNEL_SYNC_ONLY
        mono_t until;           //!< Non-zero instant when the wait will be over
#endif
        bool invert;        //!< Wait condition is inverted, i.e. we're waiting for the value to be other than @ref expect
        bool acquire;       //!< Task should acquire the masked bits (invert them) when the masked value matches @expect
        bool dynamic;       //!< Task has been allocated dynamically (not wait related)
        uintptr_t mask;         //!< Mask of bits which are checked in the byte pointed to by @ref ptr
        uintptr_t expect;       //!< Value expected at @ref ptr (after applying @ref mask)
        uintptr_t* ptr;         //!< Pointer to the value on which the task is waiting
        AsyncFrame* frame;      //!< Leaf frame that initiated the wait
    } wait;             //!< Additional parameters related to wait

    //! Runs multiple child tasks and configures the frame to wait for all of them to complete
    static async_once(RunAll, const AsyncDelegate<>*, size_t count);

public:
    //! Runs a new task on the main scheduler
    ALWAYS_INLINE static Task& Run(async_fptr_t fn);
    //! Runs a new task on the main scheduler
    ALWAYS_INLINE static Task& Run(AsyncDelegate<> fn);
    //! Runs a new task on the main scheduler
    template<typename... Args, typename... AArgs> ALWAYS_INLINE static Task& Run(async_fptr_args_t<Args...> fn, AArgs&&... args);
    //! Runs a new task on the main scheduler
    template<typename... Args, typename... AArgs> ALWAYS_INLINE static Task& Run(AsyncDelegate<Args...> fn, AArgs&&... args);
    //! Runs a new task on the main scheduler
    template<typename T, typename... Args, typename... AArgs> ALWAYS_INLINE static Task& Run(T& target, async_methodptr_t<T, Args...> method, AArgs&&... args);
    //! Runs a new task on the main scheduler
    template<typename T, typename... Args, typename... AArgs> ALWAYS_INLINE static Task& Run(T* target, async_methodptr_t<T, Args...> method, AArgs&&... args);

    //! Gets a reference to the current task
    ALWAYS_INLINE static Task& Current();

#if !KERNEL_SYNC_ONLY
    //! Delays the start of the task by the specified number of ticks, can be used only before the task is started
    ALWAYS_INLINE Task& DelayTicks(mono_t ticks) { ASSERT(!top); wait.until += ticks; return *this; }
    //! Delays the start of the task by the specified number of milliseconds, can be used only before the task is started
    ALWAYS_INLINE Task& DelayMilliseconds(mono_t ms) { ASSERT(!top); wait.until += MonoFromMilliseconds(ms); return *this; }
    //! Delays the start of the task by the specified number of seconds, can be used only before the task is started
    ALWAYS_INLINE Task& DelaySeconds(mono_t sec) { ASSERT(!top); wait.until += MonoFromSeconds(sec); return *this; }
    //! Delays the start of the task until the specified instant, can be used only before the task is started
    ALWAYS_INLINE Task& DelayUntil(mono_t instant) { ASSERT(!top); wait.until = instant; return *this; }
#endif

    //! Configures a delegate that is called when the task completes; can be used only before the task is started
    ALWAYS_INLINE Task& OnComplete(Delegate<void, intptr_t> delegate) { ASSERT(!top); onComplete = delegate; return *this; }

    template<typename... Args> ALWAYS_INLINE static async_once(RunAll, Args... delegates)
    {
        const AsyncDelegate<> tmp[] = { delegates... };
        return async_forward(RunAll, tmp, sizeof...(delegates));
    }

    //! Temporarily switches the current task to another root function, useful when it's likely many waits will occur
    static async_once(Switch, AsyncDelegate<> other);

    friend class Scheduler;
    friend struct ::AsyncFrame;
    template<typename... Args> friend class TaskWithArgs;
    template<typename... Args> friend class TaskFnWithArgs;
};

template<typename... Args> class TaskWithArgs : public Task
{
    AsyncDelegate<Args...> delegate;
    std::tuple<Args...> args;

public:
    TaskWithArgs(AsyncDelegate<Args...> delegate, Args... args)
        : delegate(delegate), args(args...)
    {
        wait.dynamic = true;
        fn = GetDelegate(this, &TaskWithArgs<Args...>::Call);
    }

private:
    FLATTEN async(Call) { return std::apply(delegate, std::tuple_cat(std::make_tuple(__pCallee), args)); }
};

template<typename... Args> class TaskFnWithArgs : public Task
{
    async_fptr_args_t<Args...> f;
    std::tuple<Args...> args;

public:
    TaskFnWithArgs(async_fptr_args_t<Args...> f, Args... args)
        : f(f), args(args...)
    {
        wait.dynamic = true;
        fn = GetDelegate(this, &TaskFnWithArgs<Args...>::Call);
    }

private:
    FLATTEN async(Call) { return std::apply(f, std::tuple_cat(std::make_tuple(__pCallee), args)); }
};


}

#include <kernel/Scheduler.h>

namespace kernel
{

ALWAYS_INLINE Task& Task::Run(async_fptr_t fn) { return Scheduler::Current().Add(fn); }
ALWAYS_INLINE Task& Task::Run(AsyncDelegate<> fn) { return Scheduler::Current().Add(fn); }
template<typename... Args, typename... AArgs> Task& Task::Run(async_fptr_args_t<Args...> fn, AArgs&&... args) { return Scheduler::Main().Add(fn, std::forward<AArgs>(args)...); }
template<typename... Args, typename... AArgs> Task& Task::Run(AsyncDelegate<Args...> fn, AArgs&&... args) { return Scheduler::Main().Add(fn, std::forward<AArgs>(args)...); }
template<typename T, typename... Args, typename... AArgs> ALWAYS_INLINE Task& Task::Run(T& target, async_methodptr_t<T, Args...> method, AArgs&&... args) { return Scheduler::Main().Add(target, method, std::forward<AArgs>(args)...); }
template<typename T, typename... Args, typename... AArgs> ALWAYS_INLINE Task& Task::Run(T* target, async_methodptr_t<T, Args...> method, AArgs&&... args) { return Scheduler::Main().Add(target, method, std::forward<AArgs>(args)...); }
ALWAYS_INLINE Task& Task::Current() { return Scheduler::Current().CurrentTask(); }

}
