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

int main()
{
    kernel::Scheduler::Main().Run();
}
