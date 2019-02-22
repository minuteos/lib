/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * targets/host/kernel/platform.h
 *
 * Kernel support for host platform
 */

#pragma once

typedef uint64_t mono_t;

#define MONO_FREQUENCY  1000000
#define MONO_CLOCKS     MONO_US

#define PLATFORM_DISABLE_INTERRUPTS()
#define PLATFORM_ENABLE_INTERRUPTS()
#define PLATFORM_SLEEP(since, duration)  __platform_sleep((since) + (duration))

#include_next <kernel/platform.h>
