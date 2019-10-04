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
#include <kernel/Scheduler.h>

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
    AsyncDelegate<> fn; //!< Function implmeneting the task
    AsyncFrame* top;    //!< Pointer to the topmost frame of the async stack
    struct
    {
        mono_t until;           //!< Instant when the wait will be over
        bool cont : 1;          //!< Indicates that the next wait should continue immediately after the time in the @ref until field
        bool invert : 1;        //!< Wait condition is inverted, i.e. we're waiting for the value to be other than @ref expect
        bool acquire : 1;       //!< Task should acquire the masked bits (invert them) when the masked value matches @expect
        unsigned index : 5;     //!< Index of the task, specifies the bit marked in completePtr on completion
        uintptr_t mask;         //!< Mask of bits which are checked in the byte pointed to by @ref ptr
        uintptr_t expect;       //!< Value expected at @ref ptr (after applying @ref mask)
        uintptr_t* ptr;         //!< Pointer to the value on which the task is waiting
        AsyncFrame* frame;      //!< Leaf frame that initiated the wait
        Task* owner;            //!< Parent task that started this one using RunAll
    } wait;             //!< Additional parameters relative to wait

    //! Runs multiple child tasks and configures the frame to wait for all of them to complete
    static async_res_t RunAll(::AsyncFrame& frame, const AsyncDelegate<>*, size_t count);

public:
    //! Runs a new task on the main scheduler, with an optional delay
    ALWAYS_INLINE static void Run(async_fptr_t fn, mono_t delay = 0) { Scheduler::Main().Add(fn, delay); }
    //! Runs a new task on the main scheduler, with an optional delay
    ALWAYS_INLINE static void Run(AsyncDelegate<> fn, mono_t delay = 0) { Scheduler::Main().Add(fn, delay); }
    //! Runs a new task on the main scheduler, with an optional delay
    template<typename T> ALWAYS_INLINE static void Run(T& target, async_methodptr_t<T> method, mono_t delay = 0) { Scheduler::Main().Add(target, method, delay); }
    //! Runs a new task on the main scheduler, with an optional delay
    template<typename T> ALWAYS_INLINE static void Run(T* target, async_methodptr_t<T> method, mono_t delay = 0) { Scheduler::Main().Add(target, method, delay); }

    template<typename... Args> ALWAYS_INLINE static async_res_t _RunAll(::AsyncFrame& frame, Args... delegates)
    {
        const AsyncDelegate<> tmp[] = { delegates... };
        return RunAll(frame, tmp, sizeof...(delegates));
    }

    friend class Scheduler;
    friend struct ::AsyncFrame;
};

}
