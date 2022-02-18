/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/config.h
 *
 * Loads and validates platform specific configuration
 */

#pragma once

#include <base/base.h>

#ifndef KERNEL_PLATFORM_HEADER
#define KERNEL_PLATFORM_HEADER kernel/platform.h
#endif

#define _KERNEL_PLATFORM_HEADER <KERNEL_PLATFORM_HEADER>

// platform-specific kernel support header
#include _KERNEL_PLATFORM_HEADER

#ifndef MONO_FREQUENCY
#error "'kernel/platform.h' did not provide a MONO_FREQUENCY macro - please check the active target"
#endif

#ifndef MONO_CLOCKS
#error "'kernel/platform.h' did not provide a MONO_CLOCKS macro - please check the active target"
#endif

#if !defined(PLATFORM_DISABLE_INTERRUPTS) || !defined(PLATFORM_ENABLE_INTERRUPTS)

#warning "'kernel/platform.h' did not provide either or both PLATFORM_DISABLE_INTERRUPTS and PLATFORM_ENABLE_INTERRUPTS macros - assuming platform without interrupts"

#ifndef PLATFORM_DISABLE_INTERRUPTS
#define PLATFORM_DISABLE_INTERRUPTS()
#endif

#ifndef PLATFORM_ENABLE_INTERRUPTS
#define PLATFORM_ENABLE_INTERRUPTS()
#endif

#endif

#ifndef PLATFORM_SLEEP
#warning "'kernel/platform.h' did not provide PLATFORM_SLEEP macro - assuming platform without sleep support"
#define PLATFORM_SLEEP(since, duration)
#endif

#if !defined(PLATFORM_DEEP_SLEEP_DISABLE) && !defined(PLATFORM_DEEP_SLEEP_ENABLE) && !defined(PLATFORM_DEEP_SLEEP_ENABLED)
#define PLATFORM_DEEP_SLEEP_DISABLE()
#define PLATFORM_DEEP_SLEEP_ENABLE()
#define PLATFORM_DEEP_SLEEP_ENABLED() (0)
#endif

#if KERNEL_STATS
#ifndef PLATFORM_CYCLE_COUNT
#warning "'kernel/platform.h' did not provide PLATFORM_CYCLE_COUNT macro, but statistics are enabled; cycle counts will be missing"
#define PLATFORM_CYCLE_COUNT 0
#endif

#if !defined(PLATFORM_WAKE_REASON_COUNT)
#warning "'kernel/platform.h' did not provide PLATFORM_WAKE_REASON(_COUNT) macro, but statistics are enabled; wake reasons will not be reported"
#define PLATFORM_WAKE_REASON_COUNT 0
#endif
#endif

#include <type_traits>
#include <limits>

typedef std::make_signed_t<mono_t> mono_signed_t;

#define MONO_SIGNED_MAX (std::numeric_limits<mono_signed_t>::max())

#include <kernel/platform_post.h>
