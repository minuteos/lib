/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * helpers.h
 *
 * Various useful helper macros
 */

#pragma once

#include <base/base.h>

//! Helper for simple call forwarding of a function taking variable arguments to one taking a va_list
#define va_call(func, last, ...) \
    ({ va_list va; va_start(va, last); auto res = func(__VA_ARGS__, va); va_end(va); res; })

//! Helper for simple call forwarding of a function taking variable arguments to one taking a va_list and returning void
#define va_call_void(func, last, ...) \
    ({ va_list va; va_start(va, last); func(__VA_ARGS__, va); va_end(va); })

//! Gets the static count of elements in an array
#define countof(arr)	(sizeof(arr) / sizeof((arr)[0]))
//! Gets the static pointer to the end of an array
#define endof(arr)      (&((arr)[countof(arr)]))

#ifdef __cplusplus

//! Forces an expression to be compile-time evaluated
template<typename T, T value> constexpr T force_constexpr() { return value; }

//! Cause a value to never be zero
template<typename T> constexpr T nonzero(T value) { return value ? value : 1; }

#endif
