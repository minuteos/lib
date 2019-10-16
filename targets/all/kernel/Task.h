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

    struct
    {
        mono_t until;           //!< Instant when the wait will be over
        bool cont : 1;          //!< Indicates that the next wait should continue immediately after the time in the @ref until field
        bool invert : 1;        //!< Wait condition is inverted, i.e. we're waiting for the value to be other than @ref expect
        bool acquire : 1;       //!< Task should acquire the masked bits (invert them) when the masked value matches @expect
        bool dynamic : 1;       //!< Task has been allocated dynamically (not wait related)
        uintptr_t mask;         //!< Mask of bits which are checked in the byte pointed to by @ref ptr
        uintptr_t expect;       //!< Value expected at @ref ptr (after applying @ref mask)
        uintptr_t* ptr;         //!< Pointer to the value on which the task is waiting
        AsyncFrame* frame;      //!< Leaf frame that initiated the wait
    } wait;             //!< Additional parameters related to wait

    //! Runs multiple child tasks and configures the frame to wait for all of them to complete
    static async_res_t RunAll(::AsyncFrame& frame, const AsyncDelegate<>*, size_t count);

public:
    //! Runs a new task on the main scheduler
    ALWAYS_INLINE static Task& Run(async_fptr_t fn);
    //! Runs a new task on the main scheduler
    ALWAYS_INLINE static Task& Run(AsyncDelegate<> fn);
    //! Runs a new task on the main scheduler
    template<typename... Args> ALWAYS_INLINE static Task& Run(AsyncDelegate<Args...> fn, Args... args);
    //! Runs a new task on the main scheduler
    template<typename T, typename... Args> ALWAYS_INLINE static Task& Run(T& target, async_methodptr_t<T, Args...> method, Args... args);
    //! Runs a new task on the main scheduler
    template<typename T, typename... Args> ALWAYS_INLINE static Task& Run(T* target, async_methodptr_t<T, Args...> method, Args... args);

    //! Delays the start of the task by the specified number of ticks, can be used only before the task is started
    ALWAYS_INLINE Task& DelayTicks(mono_t ticks) { ASSERT(!top); wait.until += ticks; return *this; }
    //! Delays the start of the task by the specified number of milliseconds, can be used only before the task is started
    ALWAYS_INLINE Task& DelayMilliseconds(mono_t ms) { ASSERT(!top); wait.until += MonoFromMilliseconds(ms); return *this; }
    //! Delays the start of the task by the specified number of seconds, can be used only before the task is started
    ALWAYS_INLINE Task& DelaySeconds(mono_t sec) { ASSERT(!top); wait.until += MonoFromSeconds(sec); return *this; }

    //! Configures a delegate that is called when the task completes; can be used only before the task is started
    ALWAYS_INLINE Task& OnComplete(Delegate<void, intptr_t> delegate) { ASSERT(!top); onComplete = delegate; return *this; }

    template<typename... Args> ALWAYS_INLINE static async_res_t _RunAll(::AsyncFrame& frame, Args... delegates)
    {
        const AsyncDelegate<> tmp[] = { delegates... };
        return RunAll(frame, tmp, sizeof...(delegates));
    }

    friend class Scheduler;
    friend struct ::AsyncFrame;
    template<typename... Args> friend class TaskWithArgs;
};

template<size_t N> struct _CallWithArgs
{
    template<typename F, typename A1, typename T, typename... A>
    static async_res_t Call(F&& f, A1&& a1, T&& t, A&&... a) { return _CallWithArgs<N-1>::Call(f, a1, t, std::get<N-1>(t), a...); }
};

template<> struct _CallWithArgs<0>
{
    template<typename F, typename A1, typename T, typename... A>
    static async_res_t Call(F&& f, A1&& a1, T&& t, A&&... a) { return f(a1, a...); }
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
    async(Call) { return _CallWithArgs<sizeof...(Args)>::Call(delegate, __pCallee, args); }
};


}

#include <kernel/Scheduler.h>

namespace kernel
{

ALWAYS_INLINE Task& Task::Run(async_fptr_t fn) { return Scheduler::Current().Add(fn); }
ALWAYS_INLINE Task& Task::Run(AsyncDelegate<> fn) { return Scheduler::Current().Add(fn); }
template<typename... Args> Task& Task::Run(AsyncDelegate<Args...> fn, Args... args) { return Scheduler::Main().Add(fn, args...); }
template<typename T, typename... Args> ALWAYS_INLINE Task& Task::Run(T& target, async_methodptr_t<T, Args...> method, Args... args) { return Scheduler::Main().Add(target, method, args...); }
template<typename T, typename... Args> ALWAYS_INLINE Task& Task::Run(T* target, async_methodptr_t<T, Args...> method, Args... args) { return Scheduler::Main().Add(target, method, args...); }

}
