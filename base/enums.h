/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * enums.h
 */

#pragma once 

#include <base/base.h>

#ifdef __cplusplus

//! Defines operators suitable for an enumeration containing flags
#define DEFINE_FLAG_ENUM(enumType) \
	ALWAYS_INLINE enumType constexpr operator |(enumType a, enumType b) { return (enumType)((int)a | (int)b); } \
	ALWAYS_INLINE enumType operator &(enumType a, enumType b) { return (enumType)((int)a & (int)b); } \
	ALWAYS_INLINE enumType operator *(enumType a, bool b) { return (enumType)((int)a * (int)b); } \
	ALWAYS_INLINE enumType operator *(bool a, enumType b) { return (enumType)((int)a * (int)b); } \
	ALWAYS_INLINE enumType operator ~(enumType a) { return (enumType)(~(int)a); } \
	ALWAYS_INLINE bool operator !(enumType a) { return !(int)a; }

#else

#define DEFINE_FLAG_ENUM(enumType)

#endif
