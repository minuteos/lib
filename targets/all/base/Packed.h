/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * Packed.h
 *
 * Support for packing structures into integer types
 *
 * This is extremely useful on 32-bit ARM MCUs, where we can return entire
 * structures of up to 4 words in registers this way
 */

#pragma once

#include <base/base.h>

template<typename T, size_t size = sizeof(T)> class _Packed
{
    using packed_t = T;
};

template<typename T, size_t size = sizeof(T)> using Packed = typename _Packed<T, size>::packed_t;

#if __ARM_PCS || __ARM_PCS_VFP

template<typename T> struct _Packed<T, 4>
{
    enum struct packed_t : uint32_t {};
};

template<typename T> struct _Packed<T, 8>
{
    enum struct packed_t : uint64_t {};
};

template<typename T> struct _Packed<T, 12>
{
    typedef uint32_t __attribute__((vector_size(16))) packed_t;
};

template<typename T> struct _Packed<T, 16>
{
    typedef uint32_t __attribute__((vector_size(16))) packed_t;
};

#endif

template<typename T> union PackedWrapper
{
    T value;
    Packed<T> packed;
};

template<typename T>
ALWAYS_INLINE static constexpr T unpack(const Packed<T>& packed)
{
    return PackedWrapper<T>{ .packed = packed }.value;
}

template<typename T, typename... Initializers>
ALWAYS_INLINE static constexpr Packed<T> pack(Initializers... init)
{
    return PackedWrapper<T>{ .value = { init... } }.packed;
}

template<typename T>
ALWAYS_INLINE static constexpr Packed<T> pack(const T& init)
{
    return PackedWrapper<T>{ .value = init }.packed;
}
