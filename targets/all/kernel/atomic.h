/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/atomic.h
 *
 * Support for atomic operations
 */

#pragma once

#include <kernel/config.h>

namespace kernel
{

template<size_t size> struct AtomicImpl
{
};

template<> struct AtomicImpl<1>
{
    typedef uint8_t value_t;
#ifdef PLATFORM_ATOMIC_EXCHANGE_8
    static value_t Exchange(value_t* target, value_t value) { return PLATFORM_ATOMIC_EXCHANGE_8(target, value); }
#endif
};

template<> struct AtomicImpl<2>
{
    typedef uint16_t value_t;
#ifdef PLATFORM_ATOMIC_EXCHANGE_16
    static value_t Exchange(value_t* target, value_t value) { return PLATFORM_ATOMIC_EXCHANGE_16(target, value); }
#endif
};

template<> struct AtomicImpl<4>
{
    typedef uint32_t value_t;
#ifdef PLATFORM_ATOMIC_EXCHANGE_32
    static value_t Exchange(value_t* target, value_t value) { return PLATFORM_ATOMIC_EXCHANGE_32(target, value); }
#endif
};

template<> struct AtomicImpl<8>
{
    typedef uint64_t value_t;
#ifdef PLATFORM_ATOMIC_EXCHANGE_64
    static value_t Exchange(value_t* target, value_t value) { return PLATFORM_ATOMIC_EXCHANGE_64(target, value); }
#endif
};

template<typename T> T AtomicExchange(T& target, T value)
{
    using impl = AtomicImpl<sizeof(T)>;
    return (T)impl::Exchange((typename impl::value_t*)&target, (typename impl::value_t)value);
}

}
