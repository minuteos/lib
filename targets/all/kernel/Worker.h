/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/Worker.h
 */

#pragma once

#include <kernel/kernel.h>

#include <base/ID.h>

namespace kernel
{

struct WorkerOptions
{
    static constexpr size_t DefaultStack = 1024;
    size_t stack = DefaultStack;
};

}

#include <kernel/PlatformWorker.h>

namespace kernel
{

class Worker
#ifdef PLATFORM_WORKER_CLASS_BASE
    : PLATFORM_WORKER_CLASS_BASE
#endif
{
public:
    template<typename TRes, typename... Args, typename... AArgs> static async_once(RunWithOptions, const WorkerOptions& opts, TRes (*fn)(Args...), AArgs&&... args);
    template<typename TRes, typename... Args, typename... AArgs> static async_once(RunWithOptions, const WorkerOptions& opts, Delegate<TRes, Args...> fn, AArgs&&... args) { return async_forward(RunWithOptions, {}, fn.FunctionPointer(), fn.Target(), args...); }
    template<typename TRes, typename... Args, typename... AArgs> static async_once(Run, TRes (*fn)(Args...), AArgs&&... args) { return async_forward(RunWithOptions, {}, fn, args...); }
    template<typename TRes, typename... Args, typename... AArgs> static async_once(Run, Delegate<TRes, Args...> fn, AArgs&&... args) { return async_forward(RunWithOptions, {}, fn, args...); }

private:
    typedef intptr_t (*fptr_run_t)(Worker* w);

    Worker(const WorkerOptions& opts, fptr_run_t run)
        :
#ifdef PLATFORM_WORKER_CLASS_BASE
            PLATFORM_WORKER_CLASS_BASE(opts),
#endif
            run(run) {}

    fptr_run_t run;

    async_once(Run);
    async(RunSync);

    template<typename TRes, typename... Args> friend class WorkerWithArgs;
    template<typename TRes, typename... Args> friend class WorkerFnWithArgs;
};

template<typename TRes, typename... Args> class WorkerFnWithArgs : public Worker
{
    using fptr_t = TRes (*)(Args...);

    fptr_t f;
    std::tuple<Args...> args;

public:
    WorkerFnWithArgs(const WorkerOptions& opts, fptr_t f, Args... args)
        : Worker(opts, &Call), f(f), args(args...)
    {
    }

private:
    static intptr_t Call(Worker* w) { return ((WorkerFnWithArgs*)w)->Call(); }
    intptr_t Call()
    {
        if constexpr (std::is_same_v<TRes, void>)
        {
            std::apply(f, args);
            return 0;
        }
        else
        {
            return std::apply(f, args);
        }
    }
};

template<typename TRes, typename... Args, typename... AArgs> async_once(Worker::RunWithOptions, const WorkerOptions& opts, TRes (*fn)(Args...), AArgs&&... args)
{
    auto worker = new(MemPoolAllocDynamic<WorkerFnWithArgs<TRes, Args...>>()) WorkerFnWithArgs<TRes, Args...>(opts, fn, std::forward<AArgs>(args)...);
    return worker->Run(__pCallee);
}

}

#include <kernel/PlatformWorkerInline.h>
