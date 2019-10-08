/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/fnv1.h
 *
 * Compile-time FNV1a implementation
 */

#pragma once

#include <base/base.h>

#define FNV1_BASIS 0x811C9DC5u
#define FNV1_PRIME 0x01000193u

//! Calculates the FNV1a hash of a null-terminated string
constexpr uint32_t fnv1a(const char* s, uint32_t hash = FNV1_BASIS)
{
    return s[0] ? fnv1a(s + 1, (hash ^ s[0]) * FNV1_PRIME) : hash;
}

//! Calculates the FNV1a hash of a block of memory
constexpr uint32_t fnv1a(const char* s, size_t len, uint32_t hash)
{
    return len ? fnv1a(s + 1, len - 1, (hash ^ s[0]) * FNV1_PRIME) : hash;
}
