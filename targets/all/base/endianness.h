/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/endianness.h
 *
 * A set of macros dealing with byte order on the target platform
 */

#pragma once

#ifdef __BYTE_ORDER__

#if defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define PLATFORM_BIG_ENDIAN      1
#define PLATFORM_LITTLE_ENDIAN   0
#elif defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define PLATFORM_BIG_ENDIAN      0
#define PLATFORM_LITTLE_ENDIAN   1
#elif !(defined(PLATFORM_BIG_ENDIAN) && defined(PLATFORM_LITTLE_ENDIAN))
#error Failed to determine platform endianness
#endif

#endif

// force specific byte order

#if PLATFORM_BIG_ENDIAN
#define TO_LE16(n)    BSWAP16(n)
#define TO_BE16(n)    (n)
#define TO_LE32(n)    BSWAP32(n)
#define TO_BE32(n)    (n)
#define FROM_LE16(n)  BSWAP16(n)
#define FROM_BE16(n)  (n)
#define FROM_LE32(n)  BSWAP32(n)
#define FROM_BE32(n)  (n)
#else
#define TO_LE16(n)    (n)
#define TO_BE16(n)    BSWAP16(n)
#define TO_LE32(n)    (n)
#define TO_BE32(n)    BSWAP32(n)
#define FROM_LE16(n)  (n)
#define FROM_BE16(n)  BSWAP16(n)
#define FROM_LE32(n)  (n)
#define FROM_BE32(n)  BSWAP32(n)
#endif
