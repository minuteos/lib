/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/bitfields.h
 *
 * Bit manipulation helpers
 */

#pragma once

#include <base/base.h>

#ifdef __cplusplus

ALWAYS_INLINE constexpr unsigned BIT(unsigned bit) { return 1 << bit; }
template<typename T> ALWAYS_INLINE constexpr bool GETBIT(const T& value, unsigned n) { return value & BIT(n); }
template<typename T> ALWAYS_INLINE constexpr T SETBIT(T& target, unsigned n) { return target |= BIT(n); }
template<typename T> ALWAYS_INLINE constexpr T RESBIT(T& target, unsigned n) { return target &= ~BIT(n); }
template<typename T> ALWAYS_INLINE constexpr T MODBIT(T& target, unsigned n, bool value) { return value ? target |= BIT(n) : target &= ~BIT(n); }

ALWAYS_INLINE constexpr unsigned MASK(unsigned bits, unsigned offset = 0) { return ((1 << bits) - 1) << offset; }
template<typename T> ALWAYS_INLINE constexpr T GETMASK(const T& value, unsigned bits, unsigned offset = 0) { return value & MASK(bits, offset); }
template<typename T> ALWAYS_INLINE constexpr T MODMASK(T& target, unsigned mask, unsigned value) { return (target = (target & ~mask) | value); }
template<typename T> ALWAYS_INLINE constexpr T MODMASK_SAFE(T& target, unsigned mask, unsigned value) { return (target = (target & ~mask) | (value & mask)); }

#endif
