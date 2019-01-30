/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/async.h
 * 
 * General support for asynchronous functions
 */

#pragma once

#include <kernel/config.h>

#include <base/MemPool.h>
#include <base/ResultPair.h>
#include <base/Delegate.h>

typedef const void* contptr_t;  //!< Pointer to the next instruction to be executed
typedef res_pair_t async_res_t; //!< Intermediate result tuple (type and associated value) of asynchronous function execution

//!< State of asynchronous function execution
enum struct AsyncResult : uintptr_t
{
    Complete,               //!< Execution has completed, value contains the result
    SleepUntil,             //!< Allow sleep until the specified instant
    SleepTicks,             //!< Allow sleep for the specified number of platform-dependent monotonic ticks
    SleepSeconds,           //!< Allow sleep for the specified number of seconds
    SleepMilliseconds,      //!< Allow sleep for the specified number of milliseconds
    DelayUntil,             //!< Unconditional sleep until the specified instant
    DelayTicks,             //!< Unconditional sleep for the specified number of platform-dependent monotonic ticks
    DelaySeconds,           //!< Unconditional sleep for the specified number of seconds
    DelayMilliseconds,      //!< Unconditional sleep for the specified number of milliseconds

    _WaitTimeoutUntil = 0x00000,
    _WaitTimeoutTicks = 0x10000,
    _WaitTimeoutSeconds = 0x20000,
    _WaitTimeoutMilliseconds = 0x30000,
    _WaitTimeoutMask = 0x30000,
    _WaitInvertedMask = 0x40000,
    _WaitAcquireMask = 0x80000,

    Wait = 0x100000,         //!< Wait for a specific byte to change to an expected value
    WaitUntil = Wait | _WaitTimeoutUntil,
    WaitTicks = Wait | _WaitTimeoutTicks,
    WaitMilliseconds = Wait | _WaitTimeoutMilliseconds,
    WaitSeconds = Wait | _WaitTimeoutSeconds,
    WaitInverted = Wait | _WaitInvertedMask,
    WaitInvertedUntil = WaitInverted | _WaitTimeoutUntil,
    WaitInvertedTicks = WaitInverted | _WaitTimeoutTicks,
    WaitInvertedMilliseconds = WaitInverted | _WaitTimeoutMilliseconds,
    WaitInvertedSeconds = WaitInverted | _WaitTimeoutSeconds,
    WaitAcquire = Wait | _WaitAcquireMask,
    WaitAcquireUntil = WaitAcquire | _WaitTimeoutUntil,
    WaitAcquireTicks = WaitAcquire | _WaitTimeoutTicks,
    WaitAcquireMilliseconds = WaitAcquire | _WaitTimeoutMilliseconds,
    WaitAcquireSeconds = WaitAcquire | _WaitTimeoutSeconds,

    _WaitEnd = 0x1FFFFF,
};

//! Extracts the value from the result tuple
#define _ASYNC_RES_VALUE(res)    RES_PAIR_FIRST(res)
//! Extracts the execution type from the result tuple
#define _ASYNC_RES_TYPE(res)     ((AsyncResult)RES_PAIR_SECOND(res))
//! Creates an asynchronous result tuple
#define _ASYNC_RES(value, type)  RES_PAIR(value, type)

//! Static definition of an asynchronous function
struct AsyncSpec
{
    MemPool* pool;      //!< Memory pool used to allocate function frames, or NULL for oversized frames
    size_t frameSize;   //!< Size of the frames required by the function
    contptr_t start;    //!< Pointer to the first actual instruction
};

//! Header for every execution frame in the asynchronous stack
struct AsyncFrame
{
    union
    {
        AsyncFrame* callee; //!< Pointer to the frame of the asynchronous function being called
        uint8_t* waitPtr;   //!< Pointer to the value to be monitored with @ref AsyncResult::Wait 
    };
    contptr_t cont;     //!< Pointer to the instruction where execution will continue
    union
    {
        mono_t waitTimeout;     //!< Timeout for the wait operation
        intptr_t waitResult;    //!< Result of the wait operation
    };
    const AsyncSpec* spec;      //!< Definition of the function to which this frame belongs

    //! Prepares the frame for a wait operation
    /*!
     * Makes sure that the mask does not cross a byte boundary,
     * initializes related fields and packs the remaining values into the result tuple
     */
    ALWAYS_INLINE async_res_t _prepare_wait(AsyncResult type, intptr_t ptr, uintptr_t mask, uintptr_t expect, intptr_t timeout)
    {
        ASSERT((intptr_t)type & (intptr_t)AsyncResult::Wait);

        auto shift = mask_shift(mask);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        waitPtr = (uint8_t*)ptr + (shift >> 3);
#else
        waitPtr = (uint8_t*)ptr;
#endif
        waitTimeout = timeout;
        expect >>= shift;
        mask >>= shift;
        ASSERT(mask == (mask & 0xFF));
        return _ASYNC_RES(this, ((intptr_t)type | (uint8_t)(mask) | (uint8_t)(expect) << 8));
    }

    static constexpr unsigned mask_shift(uintptr_t mask)
    {
        return (!mask || (mask & 0xFF)) ? 0 : (mask & 0xFF00) ? 8 : (mask & 0xFF0000) ? 16 : 24;
    }
};

//! Asynchronous function prolog
extern res_pair_t _async_prolog(AsyncFrame** pCallee, const AsyncSpec* spec);
//! Asynchronous function epilog
extern async_res_t _async_epilog(AsyncFrame** pCallee, intptr_t result);

//! Declaration of an async function
#define async(name, ...)    async_res_t name(AsyncFrame** __pCallee, ## __VA_ARGS__)

//! Implementation of a function with async interface that actually forwards to another async function
/*!
 * Usage example:
 * @code
 * ALWAYS_INLINE async(ShortDelay) { async_forward(Delay, 10); }
 * @endcode
 */
#define async_forward(name, ...)    ({ name(__pCallee, ## __VA_ARGS__); })

//! Starts the definition of an async function
/*!
 * The parameters of this macro are frame-local variables which are allocated per-invocation
 * 
 * Usage example:
 * @code
 * async(CountToN, int n)
 * async_def(int i)
 * {
 *   for (f.i = 0; f.i < n; f.i++)
 *   {
 *     async_delay_ms(200);
 *     DBG("%d\n", f.i);
 *   }
 * }
 * async_end
 * @endcode
 */
#define async_def(...) { \
    __label__ __start__; \
    struct __FRAME { AsyncFrame __async; async_res_t __epilog(AsyncFrame** pCallee, intptr_t result) { return _async_epilog(pCallee, result); } __VA_ARGS__; }; \
	static const AsyncSpec __spec = { MemPoolGet<__FRAME>(), sizeof(__FRAME), &&__start__ }; \
    auto __prolog_res = _async_prolog(__pCallee, &__spec); \
    __FRAME& f = *(__FRAME*)RES_PAIR_FIRST(__prolog_res); \
    AsyncFrame& __async = f.__async; \
    goto *(contptr_t*)RES_PAIR_SECOND(__prolog_res); \
    __start__:;

//! Starts definition of a synchronous function using the async calling convention
#define async_def_sync(...) { \
    struct __FRAME { async_res_t __epilog(AsyncFrame** pCallee, intptr_t result) { return _ASYNC_RES(result, AsyncResult::Complete); } __VA_ARGS__; } f; \

//! Terminates the definition of an async function. See @ref async_def for details
#define async_end \
    async_return(0); \
}

//! Finished the execution of an async function immediately and returns the specified value
#define async_return(value) return f.__epilog(__pCallee, (value))

#define _async_yield(type, value) ({ __label__ next; __async.cont = &&next; return _ASYNC_RES(value, AsyncResult::type); next: false; })

//! Yields execution to other tasks, but will continue as soon as possible
#define async_yield() _async_yield(SleepTicks, 0)

//! Delays execution until the specified instant
#define async_delay_until(until)  _async_yield(DelayUntil, (until))
//! Delays execution for the specified number of milliseconds
#define async_delay_ms(ms)        _async_yield(DelayMilliseconds, (ms))
//! Delays execution for the specified number of seconds
#define async_delay_sec(sec)      _async_yield(DelaySeconds, (sec))
//! Delays execution for the specified number of platform-dependent monotonic ticks
#define async_delay_ticks(ticks)  _async_yield(DelayTicks, (ticks))

//! Allows the system to sleep until the specified instant, but execution will continue as soon as the system wakes up for any reason
#define async_sleep_until(until)  _async_yield(SleepUntil, (until))
//! Allows the system to sleep for the specified number of milliseconds, but execution will continue as soon as the system wakes up for any reason
#define async_sleep_ms(ms)        _async_yield(SleepMilliseconds, (ms))
//! Allows the system to sleep for the specified number of seconds, but execution will continue as soon as the system wakes up for any reason
#define async_sleep_sec(sec)      _async_yield(SleepSeconds, (sec))
//! Allows the system to sleep for the specified number of platform-dependent monotonic ticks, but execution will continue as soon as the system wakes up for any reason
#define async_sleep_ticks(ticks)  _async_yield(SleepTicks, (ticks))

#define _await_mask(type, reg, mask, expect, timeout) ({ \
    __label__ next; \
    __async.cont = &&next; \
    return __async._prepare_wait(AsyncResult::type, (intptr_t)&(reg), (mask), (expect), (timeout)); \
next: __async.waitResult; })

//! Waits indefinitely for the value at the specified memory location to become the expected value (after masking) 
#define await_mask(reg, mask, expect)   _await_mask(Wait, reg, mask, expect, 0)
//! Waits for the value at the specified memory location to become the expected value (after masking) until the specified instant
#define await_mask_until(reg, mask, expect, until) _await_mask(WaitUntil, reg, mask, expect, until)
//! Waits for the value at the specified memory location to become the expected value (after masking) for the specified number of milliseconds
#define await_mask_ms(reg, mask, expect, ms) _await_mask(WaitMilliseconds, reg, mask, expect, ms)
//! Waits for the value at the specified memory location to become the expected value (after masking) for the specified number of seconds
#define await_mask_sec(reg, mask, expect, sec) _await_mask(WaitSeconds, reg, mask, expect, sec)
//! Waits for the value at the specified memory location to become the expected value (after masking) for the specified number of platform-dependent ticks
#define await_mask_ticks(reg, mask, expect, ticks) _await_mask(WaitTicks, reg, mask, expect, ticks)

//! Waits indefinitely for the value at the specified memory location to become other than the expected value (after masking)
#define await_mask_not(reg, mask, expect)   _await_mask(WaitInverted, reg, mask, expect, 0)
//! Waits for the value at the specified memory location to become other than the expected value (after masking) until the specified instant
#define await_mask_not_until(reg, mask, expect, until) _await_mask(WaitInvertedUntil, reg, mask, expect, until)
//! Waits for the value at the specified memory location to become other than the expected value (after masking) for the specified number of milliseconds
#define await_mask_not_ms(reg, mask, expect, ms) _await_mask(WaitInvertedMilliseconds, reg, mask, expect, ms)
//! Waits for the value at the specified memory location to become other than the expected value (after masking) for the specified number of seconds
#define await_mask_not_sec(reg, mask, expect, sec) _await_mask(WaitInvertedSeconds, reg, mask, expect, sec)
//! Waits for the value at the specified memory location to become other than the expected value (after masking) for the specified number of platform-dependent ticks
#define await_mask_not_ticks(reg, mask, expect, ticks) _await_mask(WaitInvertedTicks, reg, mask, expect, ticks)

//! Waits indefinitely for the byte at the specified memory location to become non-zero
#define await_signal(sig) await_mask_not(sig, 0xFF, 0)
//! Waits for the byte at the specified memory location to become non-zero until the specified instant
#define await_signal_until(sig, until) await_mask_not_until(sig, 0xFF, 0, until)
//! Waits for the byte at the specified memory location to become non-zero for the specified number of milliseconds
#define await_signal_ms(sig, ms) await_mask_not_ms(sig, 0xFF, 0, ms)
//! Waits for the byte at the specified memory location to become non-zero for the specified number of seconds
#define await_signal_sec(sig, sec) await_mask_not_sec(sig, 0xFF, 0, sec)
//! Waits for the byte at the specified memory location to become non-zero for the specified number of platform-dependent ticks
#define await_signal_ticks(sig, ticks) await_mask_not_ticks(sig, 0xFF, 0, ticks)

//! Waits indefinitely for the acquisition of the specified bits
#define await_acquire(reg, mask)   _await_mask(WaitAcquire, reg, mask, 0, 0)
//! Waits for the acquisition of the specified bits until the specified instant
#define await_acquire_until(reg, mask, until) _await_mask(WaitAcquireUntil, reg, mask, 0, until)
//! Waits for the acquisition of the specified bits for the specified number of milliseconds
#define await_acquire_ms(reg, mask, ms) _await_mask(WaitAcquireMilliseconds, reg, mask, 0, ms)
//! Waits for the acquisition of the specified bits for the specified number of seconds
#define await_acquire_sec(reg, mask, sec) _await_mask(WaitAcquireSeconds, reg, mask, 0, sec)
//! Waits for the acquisition of the specified bits for the specified number of platform-dependent ticks
#define await_acquire_ticks(reg, mask, ticks) _await_mask(WaitAcquireTicks, reg, mask, 0, ticks)

//! Calls another async function
#define await(fn, ...) ({ \
    __label__ next; \
    __async.cont = &&next; \
next: \
    auto res = fn(&__async.callee, ## __VA_ARGS__); \
    if (_ASYNC_RES_TYPE(res) != AsyncResult::Complete) return res; \
    _ASYNC_RES_VALUE(res); })

//! Alias for @ref Delegate to an asynchronous function
template<typename... Args> using AsyncDelegate = Delegate<async_res_t, AsyncFrame**, Args...>;

//! Pointer to an asynchronous function
typedef async_res_t (*async_fptr_t)(AsyncFrame**);
//! Pointer to an asynchronous instance method
template<class T> using async_methodptr_t = async_res_t (T::*)(AsyncFrame**);
