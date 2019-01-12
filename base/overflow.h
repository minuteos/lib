/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * overflow.h
 * 
 * Helpers for overflow arithmetic
 */

#pragma once

#include <base/base.h>

#ifdef __cplusplus

#include <type_traits>
#include <limits>

template<typename T> constexpr std::make_signed_t<T> OVF_DIFF(T a, T b) { return std::make_signed_t<T>(a - b); }
template<typename T> constexpr bool OVF_LT(T a, T b) { return OVF_DIFF(a, b) < 0; }
template<typename T> constexpr bool OVF_LE(T a, T b) { return OVF_DIFF(a, b) <= 0; }
template<typename T> constexpr bool OVF_GT(T a, T b) { return OVF_DIFF(a, b) > 0; }
template<typename T> constexpr bool OVF_GE(T a, T b) { return OVF_DIFF(a, b) >= 0; }
template<typename T> constexpr std::make_signed_t<T> OVF_MAX_DIFF() { return std::numeric_limits<std::make_signed_t<T>>::max(); }

#endif