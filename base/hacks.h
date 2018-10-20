/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * hacks.h - all the dirty tricks relying on undefined behavior in one place
 */

#include <base/base.h>

template<typename TTo, typename TFrom> 
ALWAYS_INLINE static constexpr TTo& unsafe_cast(TFrom& value)
{ return *(TTo*)&value; }

template<typename TTo, typename TFrom> 
ALWAYS_INLINE static constexpr const TTo& unsafe_cast(const TFrom& value)
{ return *(const TTo*)&value; }
