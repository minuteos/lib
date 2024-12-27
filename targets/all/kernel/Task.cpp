/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/Task.cpp
 */

#include <kernel/kernel.h>

namespace kernel
{

async_res_t Task::RunAll(::AsyncFrame& frame, const AsyncDelegate<>* delegates, size_t count)
{
    auto& scheduler = Scheduler::Current();
    auto* current = scheduler.current;

    ASSERT(count <= MaxRunAll);

    auto onComplete = GetDelegate(&frame, &AsyncFrame::_child_completed);

    for (size_t i = 0; i < count; i++)
    {
        scheduler.Add(delegates[i]).OnComplete(onComplete);
    }

    frame.waitPtr = &frame.children;
    frame.children = count;
    current->wait.mask = ~0u;
    current->wait.expect = 0;
    return _ASYNC_RES(&frame, AsyncResult::Wait);
}

async_once(Task::Switch, AsyncDelegate<> other, bool trySync)
{
    struct SwitchContext
    {
        SwitchContext(Task& t, AsyncFrame& f, AsyncDelegate<> fn, AsyncFrame* top)
            : t(t), f(f), fn(fn), prevFn(t.fn), prevTop(t.top)
        {
            t.fn = GetMethodDelegate(this, Run);
            t.top = top;
        }

        async_res_t Run(AsyncFrame** pCallee)
        {
            auto _res = fn(pCallee);
            auto res = unpack<_async_res_t>(_res);
            if (res.type == AsyncResult::Complete)
            {
                t.fn = prevFn;
                t.top = prevTop;
                f.waitResult = res.value;
                MemPoolFree(this);
                _res = _ASYNC_RES(0, AsyncResult::SleepTicks);
            }
            return _res;
        }

        Task& t;
        AsyncFrame& f;
        AsyncDelegate<> fn;
        AsyncDelegate<> prevFn;
        AsyncFrame* prevTop;
    };

    AsyncFrame* top = NULL;
    if (trySync)
    {
        auto res = other(&top);
        if (_ASYNC_RES_TYPE(res) == AsyncResult::Complete)
        {
            return res;
        }
    }
    new(MemPoolAlloc<SwitchContext>()) SwitchContext(Current(), __pCallee, other, top);
    return _ASYNC_RES(0, AsyncResult::SleepTicks);
}

}
