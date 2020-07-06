/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/mono.h
 *
 * Monotonic time conversions
 */

#pragma once

#include <kernel/config.h>

#include <ratio>

//! Ratio for converting milliseconds to monotonic ticks
typedef std::ratio<MONO_FREQUENCY, 1000> mono_ms_ratio;
//! Ratio for converting microseconds to monotonic ticks
typedef std::ratio<MONO_FREQUENCY, 1000000> mono_us_ratio;

//! Converts microseconds to platform-dependent monotonic ticks, rounding up
template<typename T> ALWAYS_INLINE static constexpr mono_t MonoFromMicroseconds(T us) { return (us * (mono_t)mono_us_ratio::num + (mono_t)mono_us_ratio::den - 1) / (mono_t)mono_us_ratio::den; }
//! Converts milliseconds to platform-dependent monotonic ticks, rounding up
template<typename T> ALWAYS_INLINE static constexpr mono_t MonoFromMilliseconds(T ms) { return (ms * (mono_t)mono_ms_ratio::num + (mono_t)mono_ms_ratio::den - 1) / (mono_t)mono_ms_ratio::den; }
//! Converts seconds to platform-dependent monotonic ticks
template<typename T> ALWAYS_INLINE static constexpr mono_t MonoFromSeconds(T sec) { return sec * MONO_FREQUENCY; }

//! Converts platform-dependent monotonic ticks to microseconds, rounding down
ALWAYS_INLINE static constexpr mono_t MonoToMicroseconds(mono_t mono) { return mono * (mono_t)mono_us_ratio::den / (mono_t)mono_us_ratio::num; }
//! Converts platform-dependent monotonic ticks to milliseconds, rounding down
ALWAYS_INLINE static constexpr mono_t MonoToMilliseconds(mono_t mono) { return mono * (mono_t)mono_ms_ratio::den / (mono_t)mono_ms_ratio::num; }
//! Converts platform-dependent monotonic ticks to seconds, rounding down
ALWAYS_INLINE static constexpr mono_t MonoToSeconds(mono_t mono) { return mono / MONO_FREQUENCY; }
