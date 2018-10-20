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

#define va_call(func, last, ...) \
	({ va_list va; va_start(va, last); auto res = func(__VA_ARGS__, va); va_end(va); res; })

#define va_call_void(func, last, ...) \
	({ va_list va; va_start(va, last); func(__VA_ARGS__, va); va_end(va); })
