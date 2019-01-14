/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/Scheduler.cpp
 *
 * Simple cooperative task scheduler
 */

#include <kernel/kernel.h>

namespace kernel
{

//! Main scheduler instance
Scheduler Scheduler::s_main;

/*!
 * Adds a new task to the scheduler
 * 
 * Tasks are always added to the @ref Scheduler::delayed queue.
 * This will cause them to actually start running in the order they are
 * added, as they are first added to the head of the delayed queue which causes
 * the order to be reversed, but the again get reversed upon transfer to the
 * active queue
 * 
 * @remark The allocated pointer to the @ref Task structure is intentionally 
 * not returned, as it's inherently dangerous to store it - the task may
 * complete at any time and if another task is created in the same cycle,
 * it will be at the same address.
 */
void Scheduler::Add(AsyncDelegate<> fn, mono_t delay)
{
    Task* t = MemPoolAlloc<Task>();
    t->fn = fn;
    t->wait.until = CurrentTime() + delay;
    t->wait.cont = true;
    t->next = delayed;
    delayed = t;
}

async_res_t Scheduler::__CallStatic(void* fptr, AsyncFrame** pCallee)
{
    return ((async_fptr_t)fptr)(pCallee);
}

/*!
 * Executes the scheduled task. Returns once there are no more tasks to execute.
 * 
 * The scheduler is very simple and repeats the following steps until terminated:
 * 
 * - all active tasks execute until they give up execution, and are processed
 *   accordingly (moved to the delayed or waiting queue, adjust maximum sleeping time)
 * - delayed tasks are made active if due
 * - if it's possible that the system will go to sleep (no more active tasks),
 *   we first check if there are any tasks remaining at all (to avoid sleeping forever),
 *   then we disable interrupts before checking the waiting tasks. This ensures that
 *   no interrupts that could wake one of the waiting tasks are missed.
 * - waiting tasks are checked, and made active if their conditions are met
 * - system goes to sleep for as long as possible, then the loop starts again
 */
mono_t Scheduler::Run()
{
    for (;;)
    {
        mono_t t = CurrentTime();
        mono_signed_t maxSleep = MONO_SIGNED_MAX;
        Task** pNext;
        Task* task;

        // first process active tasks
        pNext = &active;
        while ((task = *pNext))
        {
            auto res = task->fn(&task->top);
            auto type = _ASYNC_RES_TYPE(res);
            auto value = _ASYNC_RES_VALUE(res);
            mono_signed_t sleep;

            switch (type)
            {
                // task has finished
                case AsyncResult::Complete:
                    *pNext = task->next;
                    MemPoolFree(task);
                    continue;

                // optional sleep (task will continue in the next loop while allowing sleep up to the specified duration)
                case AsyncResult::SleepUntil: sleep = value - t; break;
                case AsyncResult::SleepMilliseconds: sleep = MonoFromMilliseconds(value); break;
                case AsyncResult::SleepSeconds: sleep = MonoFromSeconds(value); break;
                case AsyncResult::SleepTicks: sleep = value; break;

                // unconditional sleep (delay)
                case AsyncResult::DelayUntil...AsyncResult::DelayMilliseconds:
                {
                    mono_t until;
                    
                    if (task->wait.cont)
                    {
                        // continue where previous delay ended
                        until = task->wait.until;
                    }
                    else
                    {
                        task->wait.cont = true;
                        until = t;
                    }

                    switch (type)
                    {
                        case AsyncResult::DelayUntil: until = value; break;
                        case AsyncResult::DelayMilliseconds: until += MonoFromMilliseconds(value); break;
                        case AsyncResult::DelaySeconds: until += MonoFromSeconds(value); break;
                        case AsyncResult::DelayTicks: until += value; break;
                        default: ASSERT(false); break; // cannot happen
                    }

                    task->wait.until = until;

                    // move task to the delay queue
                    *pNext = task->next;
                    task->next = delayed;
                    delayed = task;
                    continue;
                }

                // waiting for a value to change
                case AsyncResult::Wait...AsyncResult::_WaitEnd:
                {
                    AsyncFrame* f = (AsyncFrame*)value;
                    task->wait.ptr = f->waitPtr;
                    task->wait.mask = (uint8_t)type;
                    task->wait.expect = (uint8_t)((uint32_t)type >> 8);
                    task->wait.invert = !!((intptr_t)type & (intptr_t)AsyncResult::_WaitInvertedMask);
                    task->wait.frame = f;

                    mono_t timeout = f->waitTimeout;
                    f->waitResult = false;
                    f->waitPtr = 0;

                    if (!timeout)
                    {
                        // we'll wait forever
                        task->wait.cont = false;
                    }
                    else
                    {
                        mono_t until;

                        if (task->wait.cont)
                        {
                            // continue where previous delay ended
                            until = task->wait.until;
                        }
                        else
                        {
                            task->wait.cont = true;
                            until = t;
                        }

                        switch ((AsyncResult)((intptr_t)type & (intptr_t)AsyncResult::_WaitTimeoutMask))
                        {
                        case AsyncResult::_WaitTimeoutUntil: until = timeout; break;
                        case AsyncResult::_WaitTimeoutMilliseconds: until += MonoFromMilliseconds(timeout); break;
                        case AsyncResult::_WaitTimeoutSeconds: until += MonoFromSeconds(timeout); break;
                        case AsyncResult::_WaitTimeoutTicks: until += timeout; break;
                        default: ASSERT(false); break; // cannot happen
                        }
                        task->wait.until = until;
                    }

                    // move task to the waiting queue
                    *pNext = task->next;
                    task->next = waiting;
                    waiting = task;
                    continue;
                }

                default:
                    sleep = 0;
                    break;
            }

            task->wait.cont = false;   // if the task did not delay again, forget the continuation

            if (maxSleep > sleep)
            {
                maxSleep = sleep;
            }

            pNext = &task->next;
        }

        // adjust for the time spent executing tasks to avoid running active tasks
        // once more even if there are tasks already due to wake up
        mono_t timeSpent = CurrentTime() - t;
        t += timeSpent;
        maxSleep -= timeSpent;

        // process delayed tasks
        pNext = &delayed;
        while ((task = *pNext))
        {
            mono_signed_t sleep = task->wait.until - t;

            if (sleep <= 0)
            {
                // return the task to the active queue
                *pNext = task->next;
                task->next = active;
                active = task;
            }
            else
            {
                pNext = &task->next;
            }

            if (maxSleep > sleep)
            {
                maxSleep = sleep;
            }
        }

        if (maxSleep > 0)
        {
            // this is a good point to check if we have any tasks remaining
            // in order to avoid the loop to sleep forever after the last task
            // completes
            if (!(active || delayed || waiting))
            {
                return t;
            }

            // disable interrupts if we're going to sleep
            // this is to make sure that an interrupt doesn't get
            // triggered between the wait check and actually going to sleep
            PLATFORM_DISABLE_INTERRUPTS();
        }

        // process waiting tasks
        pNext = &waiting;
        while ((task = *pNext))
        {
            if (((*task->wait.ptr & task->wait.mask) == task->wait.expect) != task->wait.invert)
            {
                // abort sleep and re-enable interrupts immediately to minimze latency
                if (maxSleep > 0)
                {
                    maxSleep = 0;
                    PLATFORM_ENABLE_INTERRUPTS();
                }

                // return the task to the active queue
                *pNext = task->next;
                task->next = active;
                active = task;
                task->wait.cont = false;   // make sure the next delay won't try to continue from an invalid time
                task->wait.frame->waitResult = true;
                continue;
            }

            if (task->wait.cont)
            {
                // check the timeout
                mono_signed_t sleep = task->wait.until - t;

                if (sleep <= 0)
                {
                    // abort sleep and re-enable interrupts immediately to minimze latency
                    if (maxSleep > 0)
                    {
                        maxSleep = 0;
                        PLATFORM_ENABLE_INTERRUPTS();
                    }

                    // just return the task to the active queue, the result is already set to false
                    *pNext = task->next;
                    task->next = active;
                    active = task;
                    continue;
                }
                else if (maxSleep > sleep)
                {
                    // adjust sleep interval
                    maxSleep = sleep;
                }
            }

            pNext = &task->next;
        }

        if (maxSleep > 0)
        {
            // go to sleep
            PLATFORM_SLEEP(t, maxSleep);
            PLATFORM_ENABLE_INTERRUPTS();
        }
    }
}

}
