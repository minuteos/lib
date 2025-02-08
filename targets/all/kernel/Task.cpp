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
    return _ASYNC_RES(intptr_t(&frame), AsyncResult::Wait);
}

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
        __async_res_t res = { fn(pCallee) };
        if (res.u.type <= AsyncResult::Complete)
        {
            t.fn = prevFn;
            t.top = prevTop;
            f.waitResult = res;
            MemPoolFree(this);
            res.u = { 0, AsyncResult::SleepTicks };
        }
        return res.p;
    }

    Task& t;
    AsyncFrame& f;
    AsyncDelegate<> fn;
    AsyncDelegate<> prevFn;
    AsyncFrame* prevTop;
};

async_once(Task::Switch, AsyncDelegate<> other, bool trySync)
{
    AsyncFrame* top = NULL;
    __async_res_t res = { .u = { 0, AsyncResult::SleepTicks } };
    if (trySync)
    {
        res.p = other(&top);
        if (res.u.type <= AsyncResult::Complete)
        {
            return res.p;
        }
    }
    new(MemPoolAlloc<SwitchContext>()) SwitchContext(Current(), __pCallee, other, top);
    return res.p;
}

}
