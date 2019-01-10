/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * targets/host/base/platform.h
 * 
 * Basic support for host platform
 */

#pragma once

#include <stdio.h>

typedef uint64_t mono_t;

extern void __platform_sleep(mono_t until);
extern mono_t __platform_mono_us();

#define PLATFORM_DBG_CHAR(channel, ch) putchar(ch)
#define PLATFORM_DBG_ACTIVE(channel) ((channel) == 0)

#define MONO_US __platform_mono_us()
#define MONO_US_STARTS_AT_ZERO

// provide macros used by kernel as well

#define MONO_FREQUENCY  1000000
#define MONO_CLOCKS     MONO_US

#define PLATFORM_DISABLE_INTERRUPTS()
#define PLATFORM_ENABLE_INTERRUPTS()
#define PLATFORM_SLEEP(since, duration)  __platform_sleep((since) + (duration))
