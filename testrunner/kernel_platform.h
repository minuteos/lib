/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * testrunner/kernel_platform.h
 * 
 * Replacement for kernel/platform.h for running tests, where sleeping
 * automatically jumps forward in time
 * 
 * Type sizes and monotonic frequency are chosen to be simlar to what is
 * typically used on Cortex-M MCUs, which is the primary target
 */

typedef uint32_t mono_t;

#define MONO_FREQUENCY  32768
#define MONO_CLOCKS __testrunner_time

extern mono_t __testrunner_time;

#define PLATFORM_DISABLE_INTERRUPTS()
#define PLATFORM_ENABLE_INTERRUPTS()
#define PLATFORM_SLEEP(since, duration) ({ __testrunner_time = since + duration; })
