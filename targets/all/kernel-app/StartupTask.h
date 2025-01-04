/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel-app/StartupTask.h
 */

#pragma once

#include <kernel/kernel.h>

#ifndef STARTUP_TASK_INIT_PRIORITY
#define STARTUP_TASK_INIT_PRIORITY 60000
#endif

namespace kernel
{

inline void __addStartupTask(async_fptr_t fn, mono_t delay = 0)
    { Task::Run(fn).DelayMilliseconds(delay); }

inline void __addStartupTask(AsyncDelegate<> delegate, mono_t delay = 0)
    { Task::Run(delegate).DelayMilliseconds(delay); }

template<typename T> void __addStartupTask(T& target, async_methodptr_t<T> method, mono_t delay = 0)
    { Task::Run(target, method).DelayMilliseconds(delay); }

template<typename T> void __addStartupTask(T* target, async_methodptr_t<T> method, mono_t delay = 0)
    { Task::Run(&target, method).DelayMilliseconds(delay); }

}

#define STARTUP_TASK(...) INIT_FUNCTION static void UNIQUE(__startupTask)() { ::kernel::__addStartupTask(__VA_ARGS__); }
