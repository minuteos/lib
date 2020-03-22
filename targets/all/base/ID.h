/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/ID.h
 *
 * Helper class for 32-bit IDs generated from FOURCC codes and/or FNV1a hashes of longer strings
 */

#pragma once

#include <base/fnv1.h>

class ID
{
    uint32_t id;

public:
    ALWAYS_INLINE constexpr ID(const char fourcc[5]) : id(TO_LE32(fourcc[0] | fourcc[1] << 8 | fourcc[2] << 16 | fourcc[3] << 24)) {}
    constexpr ID(uint32_t id) : id(TO_LE32(id)) {}
    constexpr ID() : id(0) {}

    constexpr bool IsValid() const { return id != 0 && id != ~0u; }	// ~0 and 0 are invalid values

    constexpr operator uint32_t() const { return FROM_LE32(id); }

    //! Create an @ref ID from a FNV1a hash of an arbitrary string literal
    static constexpr ID FNV1a(const char* s) { return fnv1a(s); }
};
