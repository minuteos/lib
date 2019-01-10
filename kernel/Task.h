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
    Task* next;         //!< Link to the next task in the queue
    AsyncDelegate<> fn; //!< Function implmeneting the task
    AsyncFrame* top;    //!< Pointer to the topmost frame of the async stack
    struct
    {
        mono_t until;           //!< Instant when the wait will be over
        bool cont;              //!< Indicates that the next wait should continue immediately after the time in the @ref until field
        bool invert;            //!< Wait condition is inverted, i.e. we're waiting for the value to be other than @ref expect
        uint8_t mask;           //!< Mask of bits which are checked in the byte pointed to by @ref ptr
        uint8_t expect;         //!< Value expected at @ref ptr (after applying @ref mask)
        const uint8_t* ptr;     //!< Pointer to the value on which the task is waiting
        AsyncFrame* frame;      //!< Leaf frame that initiated the wait
    } wait;             //!< Additional parameters relative to wait

public:
    //! Runs a new task on the main scheduler, with an optional delay
    ALWAYS_INLINE static void Run(async_res_t (*fn)(AsyncFrame**), mono_t delay = 0) { Scheduler::Main().Add(fn); }
    //! Runs a new task on the main scheduler, with an optional delay
    ALWAYS_INLINE static void Run(AsyncDelegate<> fn, mono_t delay = 0) { Scheduler::Main().Add(fn); }
    //! Runs a new task on the main scheduler, with an optional delay
    template<typename T> ALWAYS_INLINE static void Add(T& target, async_res_t (T::*method)(AsyncFrame**), mono_t delay = 0) { Scheduler::Main().Add(target, method, delay); }

    friend class Scheduler;
};

}
