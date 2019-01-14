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

namespace kernel
{

//! Simple cooperative task scheduler
class Scheduler
{
public:
    //! Adds a task to the scheduler
    void Add(AsyncDelegate<> fn, mono_t delay = 0);

    //! Adds a task represented by a static function to the scheduler
    ALWAYS_INLINE void Add(async_fptr_t function, mono_t delay = 0)
    {
        Add(AsyncDelegate<>(&__CallStatic, (void*)function), delay);
    }

    //! Syntactic helper for @ref Scheduler::Add(AsyncDelegate,mono_t)
    template<typename T> ALWAYS_INLINE void Add(T& target, async_methodptr_t<T> method, mono_t delay = 0)
    {
        Add(GetDelegate(&target, method), delay);
    }

    //! Syntactic helper for @ref Scheduler::Add(AsyncDelegate,mono_t)
    template<typename T> ALWAYS_INLINE void Add(T* target, async_methodptr_t<T> method, mono_t delay = 0)
    {
        Add(GetDelegate(target, method), delay);
    }

    //! Executes the scheduled tasks
    mono_t Run();

    //! Retrieves current monotonic time used by the scheduler
    static ALWAYS_INLINE mono_t CurrentTime() { return MONO_CLOCKS; }
    //! Retrieves the main scheduler instance
    static ALWAYS_INLINE Scheduler& Main() { return s_main; }

private:
    class Task* active = NULL;      //!< Queue of running tasks
    class Task* delayed = NULL;     //!< Queue of unconditionally sleeping tasks
    class Task* waiting = NULL;     //!< Queue of tasks waiting for a value to change

    static Scheduler s_main;        //!< Main scheduler instance

public:
    //! Wrapper for static functions to match the delegate signature
    static async_res_t __CallStatic(void* fptr, AsyncFrame** pCallee);
};

}
