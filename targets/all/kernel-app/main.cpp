/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel-app/main.cpp
 *
 * Provides a default entry point for a standard MinuteOS kernel based application
 */

#include <kernel/kernel.h>

#if !Ctestrunner
int main()
{
    if (!kernel::HardwareInit::Empty())
    {
        DBGCL("kernel-app", "Hardware init...");
        kernel::HardwareInit::Execute();
    }

    if (!kernel::StartupTask::Empty())
    {
        DBGCL("kernel-app", "%d startup tasks defined", kernel::StartupTask::Count());
        kernel::StartupTask::ScheduleAll();
    }

    DBGCL("kernel-app", "Starting main loop...");
    kernel::Scheduler::Main().Run();
}
#endif
