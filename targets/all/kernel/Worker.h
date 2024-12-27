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
    bool noPreempt : 1;
    bool trySync : 1;
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
    template<typename TRes, typename... Args, typename... AArgs> ALWAYS_INLINE static async_once(RunWithOptions, const WorkerOptions& opts, TRes (*fn)(Args...), AArgs&&... args);
    template<typename TRes, typename... Args, typename... AArgs> ALWAYS_INLINE static async_once(RunWithOptions, const WorkerOptions& opts, Delegate<TRes, Args...> fn, AArgs&&... args) { return async_forward(RunWithOptions, {}, fn.FunctionPointer(), fn.Target(), args...); }
    template<typename TRes, typename... Args, typename... AArgs> ALWAYS_INLINE static async_once(Run, TRes (*fn)(Args...), AArgs&&... args) { return async_forward(RunWithOptions, {}, fn, args...); }
    template<typename TRes, typename... Args, typename... AArgs> ALWAYS_INLINE static async_once(Run, Delegate<TRes, Args...> fn, AArgs&&... args) { return async_forward(RunWithOptions, {}, fn, args...); }

    template<typename... Args, typename... AArgs> ALWAYS_INLINE static intptr_t Await(async((*fn), Args...), AArgs&&... args) { return AwaitImpl(fn, args...); }
    template<typename... Args, typename... AArgs> ALWAYS_INLINE static intptr_t Await(async_once((*fn), Args...), AArgs&&... args) { return AwaitOnceImpl(fn, args...); }
    template<typename... Args, typename... AArgs> ALWAYS_INLINE static intptr_t Await(Delegate<async_res_t, AsyncFrame**, Args...> fn, AArgs&&... args) { return AwaitImpl(fn, args...); }
    template<typename... Args, typename... AArgs> ALWAYS_INLINE static intptr_t Await(Delegate<async_res_t, AsyncFrame&, Args...> fn, AArgs&&... args) { return AwaitOnceImpl(fn, args...); }

    static bool CanAwait();

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
    template<typename T> ALWAYS_INLINE static void Yield(T res) { YieldSync(res); }
    static void YieldSync(async_res_t res);

    template<typename Fn, typename... Args> static intptr_t AwaitImpl(Fn fn, Args&&... args)
    {
        // a regular async function allocates its own frame and we'll call it repeatedly until it completes
        AsyncFrame* pCallee = NULL;
        for (;;)
        {
            // preemption must be disabled when moving back to async world to avoid all kinds
            // of strange race conditions
            auto res = ({
                PLATFORM_CRITICAL_SECTION();
                fn(&pCallee, args...);
            });

            auto unpacked = unpack<_async_res_t>(res);
            if (unpacked.type == AsyncResult::Complete)
            {
                return unpacked.value;
            }
            else
            {
                Yield(res);
            }
        }
    }

    template<typename Fn, typename... Args> static intptr_t AwaitOnceImpl(Fn fn, Args&&... args)
    {
        // an async once function needs a full working frame (we'll provide one on stack) but can return at most one wait
        AsyncFrame frame = {};
        auto res = ({
            PLATFORM_CRITICAL_SECTION();
            fn(frame, args...);
        });

        auto unpacked = unpack<_async_res_t>(res);
        if (unpacked.type == AsyncResult::Complete)
        {
            return unpacked.value;
        }
        Yield(res);
        // the final result will wait for us in the frame
        return frame.waitResult;
    }

    template<typename TRes, typename... Args> friend class WorkerWithArgs;
    template<typename TRes, typename... Args> friend class WorkerFnWithArgs;
};

template<typename TRes, typename... Args> class WorkerFnWithArgs : public Worker
{
    using fptr_t = TRes (*)(Args...);

    fptr_t f;
    std::tuple<Args...> args;

public:
    constexpr WorkerFnWithArgs(const WorkerOptions& opts, fptr_t f, Args... args)
        : Worker(opts, &Call), f(f), args(args...)
    {
    }

private:
    FLATTEN static intptr_t Call(Worker* w) { return ((WorkerFnWithArgs*)w)->Call(); }
    FLATTEN intptr_t Call()
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

template<typename TRes, typename... Args, typename... AArgs> ALWAYS_INLINE async_once(Worker::RunWithOptions, const WorkerOptions& opts, TRes (*fn)(Args...), AArgs&&... args)
{
    auto worker = new(MemPoolAllocDynamic<WorkerFnWithArgs<TRes, Args...>>()) WorkerFnWithArgs<TRes, Args...>(opts, fn, std::forward<AArgs>(args)...);
    return worker->Run(__pCallee);
}

}

#include <kernel/PlatformWorkerInline.h>
