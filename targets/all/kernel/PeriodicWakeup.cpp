/*
 * Copyright (c) 2020 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/PeriodicWakeup.cpp
 */

#include "PeriodicWakeup.h"

namespace kernel
{

async(PeriodicWakeup::Next)
async_def_once()
{
    mono_t total = error + period;
    mono_t delay = total / fraction;
    error = total - delay * fraction;
    async_delay_until(t += delay);
}
async_end

}
