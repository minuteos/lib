/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/ResetCause.h
 *
 * Platform-independent reset cause abstraction
 */

#pragma once

#include <kernel/kernel.h>

namespace kernel
{

enum struct ResetCause
{
    PowerOn,     //< system startup after being disconnected from any power source
    Backup,      //< wakeup from backup power
    Hibernation, //< wakeup from hibernation
    Brownout,    //< insufficient power
    Watchdog,    //< watchdog timeout
    Software,    //< software request
    Hardware,    //< hardware request (RST signal, via debug interface or a button)
    MCU,         //< reset caused by MCU-specific operation
};

extern ResetCause resetCause;

}
