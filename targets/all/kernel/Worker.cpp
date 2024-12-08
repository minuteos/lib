/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/Worker.cpp
 */

#include "Worker.h"

namespace kernel
{

__attribute__((weak)) async_once(Worker::Run)
{
#if TRACE
    static struct Warn
    {
        Warn()
        {
            DBGL("WARNING! Kernel Workers not supported on the current platform - their code will by executed synchronously");
        }
    } w;
#endif

    return async_forward(Task::Switch, GetMethodDelegate(this, RunSync));
}

async(Worker::RunSync)
async_def_sync()
{
    auto res = run(this);
    MemPoolFreeDynamic(this);
    async_return(res);
}
async_end

}
