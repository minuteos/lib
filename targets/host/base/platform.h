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

extern void __platform_sleep(uint64_t until);
extern uint64_t __platform_mono_us();

#define PLATFORM_DBG_CHAR(channel, ch) putchar(ch)
#define PLATFORM_DBG_ACTIVE(channel) ((channel) == 0)

#define MONO_US __platform_mono_us()
#define MONO_US_STARTS_AT_ZERO
