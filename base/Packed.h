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

template<typename T, size_t size = sizeof(T)> union PackedWrapper
{
	typedef T packed_t;
	T value;
	packed_t packed;
};

#if __ARM_PCS == 1

// actual packing is used only with the ARM Procedure Call Standard
template<typename T> union PackedWrapper<T, 4>
{
	typedef uint32_t packed_t;
	T value;
	packed_t packed;
};

template<typename T> union PackedWrapper<T, 8>
{
	typedef uint64_t packed_t;
	T value;
	packed_t packed;
};

template<typename T> union PackedWrapper<T, 12>
{
	typedef uint32_t __attribute__((vector_size(16))) packed_t;
	T value;
	packed_t packed;
};

template<typename T> union PackedWrapper<T, 16>
{
	typedef uint32_t __attribute__((vector_size(16))) packed_t;
	T value;
	packed_t packed;
};

#endif

template<typename T> using Packed = typename PackedWrapper<T>::packed_t;

template<typename T>
ALWAYS_INLINE static constexpr T& unpack(Packed<T>& packed)
{
	return unsafe_cast<T>(packed);
}

template<typename T>
ALWAYS_INLINE static constexpr const T& unpack(const Packed<T>& packed)
{
	return unsafe_cast<T>(packed);
}

template<typename T>
ALWAYS_INLINE static constexpr Packed<T>& pack(T& value)
{
	return unsafe_cast<Packed<T>>(value);
}

template<typename T>
ALWAYS_INLINE static constexpr const Packed<T>& pack(const T& value)
{
	return unsafe_cast<Packed<T>>(value);
}
