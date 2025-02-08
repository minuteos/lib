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
    void* staticStack = NULL;
    async_res_t (*adjustResult)(async_res_t) = NULL;
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
    class WorkerAwaitResult : public AsyncCatchResult
    {
    public:
        constexpr WorkerAwaitResult(const AsyncCatchResult& acr)
            : AsyncCatchResult(acr) {}

        void Rethrow()
        {
            if (ExceptionType())
            {
                Throw(ExceptionType(), Value());
            }
        }
    };

    template<typename TRes, typename... Args, typename... AArgs> ALWAYS_INLINE static async_once(RunWithOptions, const WorkerOptions& opts, TRes (*fn)(Args...), AArgs&&... args);
    template<typename TRes, typename... Args, typename... AArgs> ALWAYS_INLINE static async_once(RunWithOptions, const WorkerOptions& opts, Delegate<TRes, Args...> fn, AArgs&&... args) { return async_forward(RunWithOptions, {}, fn.FunctionPointer(), fn.Target(), args...); }
    template<typename TRes, typename... Args, typename... AArgs> ALWAYS_INLINE static async_once(Run, TRes (*fn)(Args...), AArgs&&... args) { return async_forward(RunWithOptions, {}, fn, args...); }
    template<typename TRes, typename... Args, typename... AArgs> ALWAYS_INLINE static async_once(Run, Delegate<TRes, Args...> fn, AArgs&&... args) { return async_forward(RunWithOptions, {}, fn, args...); }

    template<typename... Args, typename... AArgs> ALWAYS_INLINE static WorkerAwaitResult Await(async((*fn), Args...), AArgs&&... args) { return AwaitImpl(fn, args...); }
    template<typename... Args, typename... AArgs> ALWAYS_INLINE static WorkerAwaitResult Await(async_once((*fn), Args...), AArgs&&... args) { return AwaitOnceImpl(fn, args...); }
    template<typename... Args, typename... AArgs> ALWAYS_INLINE static WorkerAwaitResult Await(Delegate<async_res_t, AsyncFrame**, Args...> fn, AArgs&&... args) { return AwaitImpl(fn, args...); }
    template<typename... Args, typename... AArgs> ALWAYS_INLINE static WorkerAwaitResult Await(Delegate<async_res_t, AsyncFrame&, Args...> fn, AArgs&&... args) { return AwaitOnceImpl(fn, args...); }

    static bool CanAwait();

    static void Throw(ExceptionType type, intptr_t value);

private:
    typedef async_res_t (*fptr_run_t)(Worker* w);

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

    template<typename Fn, typename... Args> static AsyncCatchResult AwaitImpl(Fn fn, Args&&... args)
    {
        // a regular async function allocates its own frame and we'll call it repeatedly until it completes
        AsyncFrame* pCallee = NULL;
        for (;;)
        {
            // preemption must be disabled when moving back to async world to avoid all kinds
            // of strange race conditions
            __async_res_t res = {({
                PLATFORM_CRITICAL_SECTION();
                fn(&pCallee, args...);
            })};

            if (res.u.type > AsyncResult::Complete)
            {
                Yield(res.p);
            }
            else
            {
                return res.p;
            }
        }
    }

    template<typename Fn, typename... Args> static AsyncCatchResult AwaitOnceImpl(Fn fn, Args&&... args)
    {
        // an async once function needs a full working frame (we'll provide one on stack) but can return at most one wait
        AsyncFrame frame = {};
        __async_res_t res = {({
            PLATFORM_CRITICAL_SECTION();
            fn(frame, args...);
        })};

        if (res.u.type > AsyncResult::Complete)
        {
            Yield(res.p);
            res = frame.waitResult;
        }

        return res.p;
    }

    template<typename TRes, typename... Args> friend class WorkerWithArgs;
    template<typename TRes, typename... Args> friend class WorkerFnWithArgs;
};

template<typename TRes, typename... Args> class WorkerFnWithArgs : public Worker
{
    using fptr_t = TRes (*)(Args...);

    fptr_t f;
    std::tuple<Args...> args;
    async_res_t (*adjustResult)(async_res_t);

public:
    constexpr WorkerFnWithArgs(const WorkerOptions& opts, fptr_t f, Args... args)
        : Worker(opts, &Call), f(f), args(args...), adjustResult(opts.adjustResult)
    {
    }

private:
    FLATTEN static async_res_t Call(Worker* w) { return ((WorkerFnWithArgs*)w)->Call(); }
    FLATTEN async_res_t Call()
    {
        async_res_t res;
        if constexpr (std::is_same_v<TRes, void>)
        {
            std::apply(f, args);
            res = {};
        }
        else
        {
            auto fRes = std::apply(f, args);
            if constexpr (std::is_same_v<TRes, async_res_t>)
            {
                res = fRes;
            }
            else
            {
                res = _ASYNC_RES(fRes, AsyncResult::Complete);
            }
            if (adjustResult)
            {
                res = adjustResult(res);
            }
        }

        return res;
    }
};

template<typename TRes, typename... Args, typename... AArgs> ALWAYS_INLINE async_once(Worker::RunWithOptions, const WorkerOptions& opts, TRes (*fn)(Args...), AArgs&&... args)
{
    auto worker = new(MemPoolAllocDynamic<WorkerFnWithArgs<TRes, Args...>>()) WorkerFnWithArgs<TRes, Args...>(opts, fn, std::forward<AArgs>(args)...);
    return worker->Run(__pCallee);
}

}

#include <kernel/PlatformWorkerInline.h>

inline NO_INLINE void kernel::Worker::Throw(ExceptionType type, intptr_t value) {
    kernel::Worker::Yield(_ASYNC_RES(value, type));
}
