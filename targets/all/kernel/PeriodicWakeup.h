/*
 * Copyright (c) 2020 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/PeriodicWakeup.h
 */

#pragma once

#include <kernel/kernel.h>

namespace kernel
{

class PeriodicWakeup
{
public:
    PeriodicWakeup(unsigned fraction, mono_t period = MONO_FREQUENCY)
        : fraction(fraction), period(period)
    {
        Reset();
    }

    //! Resets the periodic wakeup timer
    void Reset() { t = MONO_CLOCKS; error = 0; }
    //! Current tick time
    mono_t Time() const { return t; }
    //! Current accumulated error (< fraction), if any
    mono_t Error() const { return error; }
    //! Get next wakeup time (mutates state - do not use toget with async Next)
    mono_t Next();
    //! Sleeps until the next scheduled wakeup interval
    async_once(Next) async_once_def() { async_delay_until(Next()); } async_end

private:
    unsigned fraction;
    mono_t t, error, period;
};

}
