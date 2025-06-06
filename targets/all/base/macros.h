/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * macros.h - unified macros across compilers
 */

#pragma once

#if __GNUC__
#define ALWAYS_INLINE   __attribute__((always_inline, artificial)) inline
#define FLATTEN         __attribute__((flatten))
#define NO_INLINE       __attribute__((noinline))
#define UNUSED          __attribute__((unused))
#define PACKED_STRUCT   struct __attribute__((packed, aligned(sizeof(int))))
#define PACKED_UNALIGNED_STRUCT   struct __attribute__((packed, aligned(1)))
#define INIT_PRIORITY(n)      __attribute__((init_priority(20000+n)))
#define INIT_FUNCTION_PRIO(n) __attribute__((constructor(20000+n)))
#define INIT_FUNCTION         __attribute__((constructor))

#define BSWAP16 __builtin_bswap16
#define BSWAP32 __builtin_bswap32

#endif

#if __GNUC__ && !__clang__
#define OPTIMIZE_SIZE   __attribute__((optimize("-Os")))
#define OPTIMIZE        __attribute__((optimize("-O3"), optimize("-Os")))
#define OPTIMIZE_SPEED  __attribute__((optimize("-O3")))
#endif

// safe fallbacks
#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE   inline
#endif

#ifndef FLATTEN
#define FLATTEN
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

#ifndef RELEASE_ALWAYS_INLINE
#if DEBUG
#define RELEASE_ALWAYS_INLINE NO_INLINE
#else
#define RELEASE_ALWAYS_INLINE ALWAYS_INLINE
#endif
#endif

#ifndef OPTIMIZE_SIZE
#define OPTIMIZE_SIZE
#endif

#ifndef OPTIMIZE
#define OPTIMIZE
#endif

#ifndef OPTIMIZE_SPEED
#define OPTIMIZE_SPEED
#endif

#ifndef UNUSED
#define UNUSED
#endif

#ifdef __cplusplus
#define _CONSTEXPR  constexpr
#else
#define _CONSTEXPR
#endif

#ifndef BSWAP16
ALWAYS_INLINE static _CONSTEXPR uint16_t BSWAP16(uint16_t n)
{
    return (n >> 8) | (n << 8);
}
#endif

#ifndef BSWAP32
ALWAYS_INLINE static _CONSTEXPR uint32_t BSWAP32(uint32_t n)
{
    return (BSWAP16(n) << 16) | BSWAP16(n >> 16);
}
#endif

#ifndef BSWAP24
ALWAYS_INLINE static _CONSTEXPR uint32_t BSWAP24(uint32_t n)
{
    return BSWAP32(n << 8);
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

#define STRINGIFY(a) _STRINGIFY(a)
#define _STRINGIFY(a) #a

//! create an identifier unique within the compilation unit
#define UNIQUE(base)    CONCAT(base, __COUNTER__)
