/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/Task.cpp
 */

#include "Task.h"

namespace kernel
{

async_res_t Task::RunAll(::AsyncFrame& frame, const AsyncDelegate<>* delegates, size_t count)
{
    auto& scheduler = Scheduler::Current();
    auto* current = scheduler.current;

    ASSERT(count <= MaxRunAll);

    for (size_t i = 0; i < count; i++)
    {
        auto t = scheduler._Add(delegates[i], 0);
        t->wait.index = i;
        t->wait.owner = current;
    }

    frame.waitPtr = &frame.children;
    frame.children = 0;
    current->wait.mask = current->wait.expect = MASK(count);
    return _ASYNC_RES(&frame, AsyncResult::Wait);
}

}
