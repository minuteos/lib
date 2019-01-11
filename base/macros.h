/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * macros.h - unified macros across compilers
 */

#pragma once

#if __GNUC__
#define ALWAYS_INLINE   __attribute__((always_inline)) inline
#define NO_INLINE       __attribute__((noinline))
#endif

#if __GNUC__ && !__clang__
#define OPTIMIZE_SIZE   __attribute__((optimize("-Os")))
#endif

// safe fallbacks
#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE   inline
#endif

#ifndef NO_INLINE
#define NO_INLINE
#endif

#ifndef OPTIMIZE_SIZE
#define OPTIMIZE_SIZE
#endif

#ifndef OPTIMIZE
#define OPTIMIZE        OPTIMIZE_SIZE
#endif

// C/C++ agnostic EXTERN_C section
#ifdef __cplusplus
#define BEGIN_EXTERN_C  extern "C" {
#define END_EXTERN_C    }
#define EXTERN_C        extern "C"
#else
#define BEGIN_EXTERN_C
#define END_EXTERN_C
#define EXTERN_C
#endif
