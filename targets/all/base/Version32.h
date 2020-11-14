/*
 * Copyright (c) 2020 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/Version32.h
 *
 * Simple directly comparable 32-bit version number
 */

#pragma once

#include <base/base.h>

class Version32
{
    union
    {
        uint32_t raw;
        struct
        {
#if PLATFORM_BIG_ENDIAN
            uint8_t maj, min, rev, patch;
#else
            uint8_t patch, rev, min, maj;
#endif
        };
    };

public:
    Version32() = default;
    constexpr Version32(uint32_t raw)
        : raw(raw) {}
    constexpr Version32(uint8_t maj, uint8_t min, uint8_t rev, uint8_t patch)
        : raw(maj << 24 | min << 16 | rev << 8 | patch) {}

    constexpr uint8_t Major() const { return maj; }
    constexpr uint8_t Minor() const { return min; }
    constexpr uint8_t Revision() const { return rev; }
    constexpr uint8_t Patch() const { return patch; }

    constexpr bool operator <(const Version32& other) { return raw < other.raw; }
    constexpr bool operator <=(const Version32& other) { return raw <= other.raw; }
    constexpr bool operator >(const Version32& other) { return raw > other.raw; }
    constexpr bool operator >=(const Version32& other) { return raw >= other.raw; }
    constexpr bool operator ==(const Version32& other) { return raw == other.raw; }
    constexpr bool operator !=(const Version32& other) { return raw != other.raw; }

    struct Expanded
    {
        const int32_t maj, min, rev, patch;
    };

    //! Returns the version number expanded to four integers that can be used directly as formatting argument
    constexpr Expanded Expand() const { return { maj, min, rev, patch }; }
};
