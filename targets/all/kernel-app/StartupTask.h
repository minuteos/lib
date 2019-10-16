/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel-app/StartupTask.h
 */

#pragma once

#include <kernel/kernel.h>

#include <kernel-app/InitList.h>

namespace kernel
{

class StartupTask : public InitList<StartupTask>
{
    AsyncDelegate<> task;
    mono_t delay;

public:
    StartupTask(async_fptr_t fn, mono_t delay = 0)
        : task(Scheduler::__CallStatic, (void*)fn), delay(delay) {}

    StartupTask(AsyncDelegate<> delegate, mono_t delay = 0)
        : task(delegate), delay(delay) {}

    template<class T> StartupTask(T& target, async_methodptr_t<T> method, mono_t delay = 0)
        : task(&target, method), delay(delay) {}

    template<class T> StartupTask(T* target, async_methodptr_t<T> method, mono_t delay = 0)
        : task(&target, method), delay(delay) {}

    static void ScheduleAll()
    {
        for (auto t = InitList<StartupTask>::First(); t; t = t->Next())
            Task::Run(t->task).DelayMilliseconds(t->delay);
    }
};

}

#define STARTUP_TASK(...) static kernel::StartupTask UNIQUE(__startupTask)(__VA_ARGS__)
