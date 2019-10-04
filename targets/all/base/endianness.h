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

#if defined(__BIG_ENDIAN__) && (__BYTE_ORDER__ == __BIG_ENDIAN__)
#define BIG_ENDIAN      1
#define LITTLE_ENDIAN   0
#elif defined(__LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __LITTLE_ENDIAN__)
#define BIG_ENDIAN      0
#define LITTLE_ENDIAN   1
#elif !(defined(BIG_ENDIAN) && defined(LITTLE_ENDIAN))
#error Failed to determine platform endianness
#endif

#endif

// force specific byte order

#if BIG_ENDIAN
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
