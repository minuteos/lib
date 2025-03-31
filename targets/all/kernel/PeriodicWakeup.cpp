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

mono_t PeriodicWakeup::Next()
{
    mono_t total = error + period;
    mono_t delay = total / fraction;
    error = total - delay * fraction;
    return t += delay;
}

}
