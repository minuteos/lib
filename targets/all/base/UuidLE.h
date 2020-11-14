/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/UuidLE.h
 *
 * Helper class for working with UUIDs encoded in little endian order
 *
 * example:
 * 123e4567-e89b-12d3-a456-426655440000
 *
 * appears in memory as
 * 00 00 44 55 66 42 56 A4 D3 12 9B E8 67 45 3E 12
 */

#pragma once

#include <base/base.h>

class UuidLE
{
    union { struct { uint8_t j, i, h, g; }; uint32_t ghij; };
    union { struct { uint8_t e, f; }; uint16_t ef; };
    uint16_t d, c, b;
    uint32_t a;

    static constexpr uint8_t parse4(char p) { return p - (p < 'A' ? '0' : (p >= 'a' ? 'a' : 'A') - 10); }
    static constexpr uint8_t parse8(const char* p) { return (parse4(p[0]) << 4) | parse4(p[1]); }
#if PLATFORM_LITTLE_ENDIAN
    static constexpr uint16_t parse16(const char* p) { return (parse8(p) << 8) | parse8(p + 2); }
    static constexpr uint32_t parse32(const char* p) { return (parse16(p) << 16) | parse16(p + 4); }
#else
    static constexpr uint16_t parse16(const char* p) { return (parse8(p + 2) << 8) | parse8(p); }
    static constexpr uint32_t parse32(const char* p) { return (parse16(p + 4) << 16) | parse16(p); }
#endif

public:
    UuidLE() = default;
    constexpr UuidLE(const char str[37])
        : ghij(parse32(str + 28)), ef(parse16(str + 24)), d(parse16(str + 19)), c(parse16(str + 14)), b(parse16(str + 9)), a(parse32(str)) {}

    constexpr bool operator ==(const UuidLE& other) { return !memcmp(this, &other, sizeof(UuidLE)); }
    constexpr bool operator !=(const UuidLE& other) { return !!memcmp(this, &other, sizeof(UuidLE)); }
};
