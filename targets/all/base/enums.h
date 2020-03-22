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

//! Declares friend operators suitable for an enumeration containing flags
/*! This has to be used for private enums within classes */
#define DECLARE_FLAG_ENUM(enumType) \
    friend enumType constexpr operator |(enumType a, enumType b); \
    friend enumType operator |=(enumType& a, enumType b); \
    friend enumType operator &(enumType a, enumType b); \
    friend enumType operator &=(enumType& a, enumType b); \
    friend enumType operator ^(enumType a, enumType b); \
    friend enumType operator ^=(enumType& a, enumType b); \
    friend enumType operator *(enumType a, bool b); \
    friend enumType operator *(bool a, enumType b); \
    friend enumType operator ~(enumType a); \
    friend bool operator !(enumType a);

//! Defines operators suitable for an enumeration containing flags
#define DEFINE_FLAG_ENUM(enumType) \
    ALWAYS_INLINE enumType constexpr operator |(enumType a, enumType b) { return (enumType)((int)a | (int)b); } \
    ALWAYS_INLINE enumType operator |=(enumType& a, enumType b) { return a = a | b; } \
    ALWAYS_INLINE enumType operator &(enumType a, enumType b) { return (enumType)((int)a & (int)b); } \
    ALWAYS_INLINE enumType operator &=(enumType& a, enumType b) { return a = a & b; } \
    ALWAYS_INLINE enumType operator ^(enumType a, enumType b) { return (enumType)((int)a ^ (int)b); } \
    ALWAYS_INLINE enumType operator ^=(enumType& a, enumType b) { return a = a ^ b; } \
    ALWAYS_INLINE enumType operator *(enumType a, bool b) { return (enumType)((int)a * (int)b); } \
    ALWAYS_INLINE enumType operator *(bool a, enumType b) { return (enumType)((int)a * (int)b); } \
    ALWAYS_INLINE enumType operator ~(enumType a) { return (enumType)(~(int)a); } \
    ALWAYS_INLINE bool operator !(enumType a) { return !(int)a; }

#else

#define DECLARE_FLAG_ENUM(enumType)
#define DEFINE_FLAG_ENUM(enumType)

#endif
