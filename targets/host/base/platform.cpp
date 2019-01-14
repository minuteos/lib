/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * targets/host/base/platform.cpp
 * 
 * Basic support for host platform, implemented using standard C++ library.
 */

#include <base/base.h>

#include "platform.h"

#include <chrono>
#include <thread>

static std::chrono::steady_clock::time_point __steady_clock_zero()
{
    static auto __zero = std::chrono::steady_clock::now();
    return __zero;
}

static auto __zeroinit = __steady_clock_zero();

mono_t __platform_mono_us()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - __steady_clock_zero()).count();
}

void __platform_sleep(mono_t until)
{
    std::this_thread::sleep_until(__steady_clock_zero() + std::chrono::microseconds(until));
}
