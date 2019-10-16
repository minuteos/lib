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

}
