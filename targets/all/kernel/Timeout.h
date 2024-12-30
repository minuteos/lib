/*
 * Copyright (c) 2020 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/Timeout.h
 */

#pragma once

#include <kernel/config.h>

#include <kernel/mono.h>

//! Just a token representing infinite timeout
struct InfiniteTimeout
{
    constexpr InfiniteTimeout() {}
};

namespace kernel
{
    class Scheduler;
}

//! Represents an absolute or relative timeout
class Timeout
{
public:
    constexpr Timeout()
        : value(0) {}
    constexpr Timeout(const InfiniteTimeout& infinite)
        : value(0) {}

    static constexpr InfiniteTimeout Infinite = InfiniteTimeout();

    static constexpr Timeout Absolute(mono_t instant) { return Timeout(instant | (mono_t(MONO_SIGNED_MAX) + 1)); }
    static constexpr Timeout Ticks(mono_t value) { ASSERT(value <= MONO_SIGNED_MAX); return Timeout(value); }
    template<typename T> static constexpr Timeout Microseconds(T value) { return Ticks(MonoFromMicroseconds(value)); }
    template<typename T> static constexpr Timeout Milliseconds(T value) { return Ticks(MonoFromMilliseconds(value)); }
    template<typename T> static constexpr Timeout Seconds(T value) { return Ticks(MonoFromSeconds(value)); }

    constexpr bool IsInfinite() const { return value == 0; }
    constexpr bool IsAbsolute() const { return value > MONO_SIGNED_MAX; }
    constexpr bool IsRelative() const { return value <= MONO_SIGNED_MAX; }
    Timeout MakeAbsolute() const { return Timeout(IsAbsolute() || IsInfinite() ? value : ((MONO_CLOCKS + value) | (mono_t(MONO_SIGNED_MAX) + 1))); }

    bool Elapsed() const { return Relative() < 0; }
    ALWAYS_INLINE mono_signed_t Relative() const { return IsRelative() ? value : mono_signed_t((value - MONO_CLOCKS) << 1) >> 1; }
    ALWAYS_INLINE mono_signed_t Relative(mono_t to) const { return IsRelative() ? value : mono_signed_t((value - to) << 1) >> 1; }
    ALWAYS_INLINE mono_t ToMono(mono_t base = MONO_CLOCKS) const { return base + Relative(base); }
    static constexpr mono_t __raw_value(const Timeout& timeout) { return timeout.value; }

    bool operator <(const Timeout& other) const
    {
        // Infinite is never less than anything
        if (IsInfinite())
            return false;
        if (other.IsInfinite())
            return true;
        auto rel = MONO_CLOCKS;
        return Relative(rel) < other.Relative(rel);
    }

    bool operator >=(const Timeout& other) const { return !(*this < other); }
    bool operator >(const Timeout& other) const { return other < *this; }
    bool operator <=(const Timeout& other) const { return !(other < *this); }

    constexpr bool operator ==(const Timeout& other) const { return value == other.value; }
    constexpr bool operator !=(const Timeout& other) const { return value != other.value; }

    constexpr Timeout operator ||(const Timeout& other) const { return value ? *this : other; }

private:
    constexpr Timeout(mono_t value)
        : value(value) {}

    mono_t value;

    friend class kernel::Scheduler;
};

#define OPT_TIMEOUT_ARG  ::Timeout timeout = ::Timeout::Infinite
