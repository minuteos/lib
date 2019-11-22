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
#define UNUSED          __attribute__((unused))
#define PACKED_STRUCT   struct __attribute__((packed, aligned(sizeof(int))))
#define PACKED_UNALIGNED_STRUCT   struct __attribute__((packed, aligned(1)))

#define BSWAP16 __builtin_bswap16
#define BSWAP32 __builtin_bswap32

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

#ifndef DEBUG_NO_INLINE
#if DEBUG
#define DEBUG_NO_INLINE NO_INLINE
#else
#define DEBUG_NO_INLINE
#endif
#endif

#ifndef OPTIMIZE_SIZE
#define OPTIMIZE_SIZE
#endif

#ifndef OPTIMIZE
#define OPTIMIZE        OPTIMIZE_SIZE
#endif

#ifndef UNUSED
#define UNUSED
#endif

#ifndef BSWAP16
ALWAYS_INLINE static uint16_t BSWAP16(uint16_t n)
{
    return (n >> 8) | (n << 8);
}
#endif

#ifndef BSWAP32
ALWAYS_INLINE static uint32_t BSWAP32(uint32_t n)
{
    return (BSWAP16(n) << 16) | BSWAP16(n >> 16);
}
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

//! expand and concatenate parameters
#define CONCAT(a, b) _CONCAT(a, b)
#define _CONCAT(a, b) a ## b

//! create an identifier unique within the compilation unit
#define UNIQUE(base)    CONCAT(base, __COUNTER__)
