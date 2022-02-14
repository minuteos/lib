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

#include <base/base.h>

#include <base/MemPool.h>
#include <base/ResultPair.h>
#include <base/Delegate.h>

#include <kernel/Timeout.h>

typedef const void* contptr_t;  //!< Pointer to the next instruction to be executed
typedef res_pair_t async_res_t; //!< Intermediate result tuple (type and associated value) of asynchronous function execution

//!< State of asynchronous function execution
enum struct AsyncResult : intptr_t
{
    Complete,               //!< Execution has completed, value contains the result
    SleepTimeout,           //!< Allow sleep until the specified timeout elapses
    SleepUntil,             //!< Allow sleep until the specified instant
    SleepTicks,             //!< Allow sleep for the specified number of platform-dependent monotonic ticks
    SleepSeconds,           //!< Allow sleep for the specified number of seconds
    SleepMilliseconds,      //!< Allow sleep for the specified number of milliseconds
    DelayTimeout,           //!< Unconditional sleep until the specified timeout elapses
    DelayUntil,             //!< Unconditional sleep until the specified instant
    DelayTicks,             //!< Unconditional sleep for the specified number of platform-dependent monotonic ticks
    DelaySeconds,           //!< Unconditional sleep for the specified number of seconds
    DelayMilliseconds,      //!< Unconditional sleep for the specified number of milliseconds

    WaitMultiple,           //!< Wait for multiple child tasks to finish

    _WaitInvertedMask = 0x1,
    _WaitAcquireMask = 0x2,
    _WaitSignalMask = 0x4,

    Wait = 0x10,            //!< Wait for a specific word to change to an expected value
    WaitInverted = Wait | _WaitInvertedMask,
    WaitAcquire = Wait | _WaitAcquireMask,
    WaitSignal = Wait | _WaitSignalMask | _WaitInvertedMask,
    WaitInvertedSignal = Wait | _WaitSignalMask,

    _WaitEnd = 0x17
};

//! Extracts the value from the result tuple
#define _ASYNC_RES_VALUE(res)    RES_PAIR_FIRST(res)
//! Extracts the execution type from the result tuple
#define _ASYNC_RES_TYPE(res)     ((AsyncResult)RES_PAIR_SECOND(res))
//! Creates an asynchronous result tuple
#define _ASYNC_RES(value, type)  RES_PAIR(value, type)
//! Filters the asynchronous result type for a set of flags
constexpr static AsyncResult operator &(AsyncResult type, AsyncResult flag)
{
    return (AsyncResult)((intptr_t)(type) & (intptr_t)(flag));
}
//! Checks if the asynchronous result type contains a particular flag
constexpr static bool operator &&(AsyncResult type, AsyncResult flag)
{
    return (type & flag) != AsyncResult::Complete;
}

//! Static definition of an asynchronous function
struct AsyncSpec
{
    class MemPool* pool;//!< Memory pool used to allocate function frames, or NULL for oversized frames
    size_t frameSize;   //!< Size of the frames required by the function
    contptr_t start;    //!< Pointer to the first actual instruction
};

//! Header for every execution frame in the asynchronous stack
struct AsyncFrame
{
    union
    {
        AsyncFrame* callee; //!< Pointer to the frame of the asynchronous function being called
        uintptr_t* waitPtr; //!< Pointer to the value to be monitored with @ref AsyncResult::Wait
    };
    contptr_t cont;     //!< Pointer to the instruction where execution will continue
    union
    {
        mono_t waitTimeout;     //!< Timeout for the wait operation (actually a Timeout::value)
        intptr_t waitResult;    //!< Result of the wait operation
        uintptr_t children;     //!< Count of child tasks still executing
    };
    const AsyncSpec* spec;      //!< Definition of the function to which this frame belongs

    //! Prepares the frame for a wait operation
    RES_PAIR_DECL_ATTRIBUTE async_res_t _prepare_wait(AsyncResult type, uintptr_t mask, uintptr_t expect);
    //! Prepares the frame for a byte wait operation
    RES_PAIR_DECL_ATTRIBUTE async_res_t _prepare_wait(AsyncResult type);
    //! Decrements the running child count
    void _child_completed(intptr_t res);
};

//! Asynchronous function prolog
extern RES_PAIR_DECL(_async_prolog, AsyncFrame** pCallee, const AsyncSpec* spec);
//! Asynchronous function epilog
extern RES_PAIR_DECL(_async_epilog, AsyncFrame** pCallee, intptr_t result);

//! Declaration of an async function
#define async(name, ...)    RES_PAIR_DECL_ATTRIBUTE async_res_t name(AsyncFrame** __pCallee, ## __VA_ARGS__)

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
    struct __FRAME { AsyncFrame __async; \
        async_res_t __epilog(AsyncFrame** pCallee, intptr_t result) { return _async_epilog(pCallee, result); } \
        void __continue(void* cont) { __async.cont = cont; } \
        void __requireContinue(void* cont) { __async.cont = cont; } \
        __VA_ARGS__; }; \
    static const AsyncSpec __spec = { MemPoolGet<__FRAME>(), sizeof(__FRAME), &&__start__ }; \
    auto __prolog_res = _async_prolog(__pCallee, &__spec); \
    __FRAME& f = *(__FRAME*)RES_PAIR_FIRST(__prolog_res); \
    AsyncFrame& __async = f.__async; \
    goto *(contptr_t*)RES_PAIR_SECOND(__prolog_res); \
    __start__: new(&f) __FRAME;

//! Construction-time initialization of fields inside the async frame
#define async_def_init(...) __FRAME() : __VA_ARGS__ {}

//! Starts definition of a synchronous function using the async calling convention
#define async_def_sync(...) { \
    struct __FRAME { \
        async_res_t __epilog(AsyncFrame** pCallee, intptr_t result) { return _ASYNC_RES(result, AsyncResult::Complete); } \
        __VA_ARGS__; } f; \

//! Defines a simple synchronous function immediately returning a value, using the async calling convention
#define async_def_return(value) { return _ASYNC_RES(value, AsyncResult::Complete); }

//! Defines a lightweight asynchronous function without a persistent frame and automatic continuation, just call count
#define async_def_lite(...) { \
    struct __FRAME { \
        async_res_t __epilog(AsyncFrame** pCallee, intptr_t result) { *pCallee = NULL; return _ASYNC_RES(result, AsyncResult::Complete); } \
        void __continue(void* cont) { /* discard */ } \
        __VA_ARGS__; } f; \
    UNUSED const uintptr_t async_count = (*(uintptr_t*)__pCallee)++; \
    AsyncFrame& __async = *(AsyncFrame*)__pCallee;

//! Defines a lightweight asynchronous function called just once
//! The function can end with a wait operation
#define async_def_once(...) { \
    if (*__pCallee) { auto res = (*__pCallee)->waitResult; *__pCallee = NULL; return _ASYNC_RES(res, AsyncResult::Complete); } \
    *__pCallee = (AsyncFrame*)__pCallee; \
    struct __FRAME { \
        async_res_t __epilog(AsyncFrame** pCallee, intptr_t result) { *pCallee = NULL; return _ASYNC_RES(result, AsyncResult::Complete); } \
        void __continue(void* cont) { /* discard */ } \
        __VA_ARGS__; } f; \
    AsyncFrame& __async = *(AsyncFrame*)__pCallee;

//! Defines an asynchronous function with variable args that forwards to another async function with va_list
#define async_def_va(func, last, ...) { \
    va_list __va; va_start(__va, last); \
    auto res = func(__pCallee, ## __VA_ARGS__, __va); \
    va_end(__va); return res; }

//! Defines an asynchronous function with variable args that forwards to another async function
//! with va_list not being the last argument, but specified explicitly
#define async_def_va_ext(func, last, ...) { \
    va_list va; va_start(va, last); \
    auto res = func(__pCallee, ## __VA_ARGS__); \
    va_end(va); return res; }

//! Terminates the definition of an async function. See @ref async_def for details
#define async_end \
    async_return(0); \
}

//! Finished the execution of an async function immediately and returns the specified value
#define async_return(value) ({ async_res_t __res; __res = f.__epilog(__pCallee, (value)); f.~__FRAME(); return __res; })

#define _async_yield(type, value) ({ __label__ next; (void)__async; f.__continue(&&next); return _ASYNC_RES(value, AsyncResult::type); next: false; })

//! Yields execution to other tasks, but will continue as soon as possible
#define async_yield() _async_yield(SleepTicks, 0)

//! Delays execution until the specified timeout elapses
#define async_delay_timeout(timeout)  _async_yield(DelayTimeout, Timeout::__raw_value(timeout))
//! Delays execution until the specified instant
#define async_delay_until(until)  _async_yield(DelayUntil, (until))
//! Delays execution for the specified number of milliseconds
#define async_delay_ms(ms)        _async_yield(DelayMilliseconds, (ms))
//! Delays execution for the specified number of seconds
#define async_delay_sec(sec)      _async_yield(DelaySeconds, (sec))
//! Delays execution for the specified number of platform-dependent monotonic ticks
#define async_delay_ticks(ticks)  _async_yield(DelayTicks, (ticks))

//! Allows the system to sleep until the specified timeout elapses, but execution will continue as soon as the system wakes up for any reason
#define async_sleep_timeout(timeout)  _async_yield(SleepTimeout, Timeout::__raw_value(timeout))
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
    f.__continue(&&next); \
    __async.waitPtr = (uintptr_t*)&(reg); \
    __async.waitTimeout = Timeout::__raw_value(timeout); \
    { auto res = __async._prepare_wait(AsyncResult::type, (uintptr_t)(mask), (uintptr_t)(expect)); \
    if (_ASYNC_RES_TYPE(res) != AsyncResult::Complete) return res; } \
next: __async.waitResult; })

#define _await_signal(type, reg, timeout) ({ \
    __label__ next; \
    f.__continue(&&next); \
    __async.waitPtr = (uintptr_t*)&(reg); \
    __async.waitTimeout = Timeout::__raw_value(timeout); \
    { auto res = __async._prepare_wait(AsyncResult::type); \
    if (_ASYNC_RES_TYPE(res) != AsyncResult::Complete) return res; } \
next: __async.waitResult; })

//! Waits indefinitely for the value at the specified memory location to become the expected value (after masking)
#define await_mask(reg, mask, expect)   _await_mask(Wait, reg, mask, expect, Timeout::Infinite)
//! Waits for the value at the specified memory location to become the expected value (after masking) with the specified timeout
#define await_mask_timeout(reg, mask, expect, timeout) _await_mask(Wait, reg, mask, expect, timeout)
//! Waits for the value at the specified memory location to become the expected value (after masking) until the specified instant
#define await_mask_until(reg, mask, expect, until) _await_mask(Wait, reg, mask, expect, Timeout::Absolute(until))
//! Waits for the value at the specified memory location to become the expected value (after masking) for the specified number of milliseconds
#define await_mask_ms(reg, mask, expect, ms) _await_mask(Wait, reg, mask, expect, Timeout::Milliseconds(ms))
//! Waits for the value at the specified memory location to become the expected value (after masking) for the specified number of seconds
#define await_mask_sec(reg, mask, expect, sec) _await_mask(Wait, reg, mask, expect, Timeout::Seconds(sec))
//! Waits for the value at the specified memory location to become the expected value (after masking) for the specified number of platform-dependent ticks
#define await_mask_ticks(reg, mask, expect, ticks) _await_mask(Wait, reg, mask, expect, Timeout::Ticks(ticks))

//! Waits indefinitely for the value at the specified memory location to become other than the expected value (after masking)
#define await_mask_not(reg, mask, expect)   _await_mask(WaitInverted, reg, mask, expect, Timeout::Infinite)
//! Waits for the value at the specified memory location to become other than the expected value (after masking) with the specified timeout
#define await_mask_not_timeout(reg, mask, expect, timeout) _await_mask(WaitInverted, reg, mask, expect, timeout)
//! Waits for the value at the specified memory location to become other than the expected value (after masking) until the specified instant
#define await_mask_not_until(reg, mask, expect, until) _await_mask(WaitInverted, reg, mask, expect, Timeout::Absolute(until))
//! Waits for the value at the specified memory location to become other than the expected value (after masking) for the specified number of milliseconds
#define await_mask_not_ms(reg, mask, expect, ms) _await_mask(WaitInverted, reg, mask, expect, Timeout::Milliseconds(ms))
//! Waits for the value at the specified memory location to become other than the expected value (after masking) for the specified number of seconds
#define await_mask_not_sec(reg, mask, expect, sec) _await_mask(WaitInverted, reg, mask, expect, Timeout::Seconds(sec))
//! Waits for the value at the specified memory location to become other than the expected value (after masking) for the specified number of platform-dependent ticks
#define await_mask_not_ticks(reg, mask, expect, ticks) _await_mask(WaitInverted, reg, mask, expect, Timeout::Ticks(ticks))

//! Waits indefinitely for the byte at the specified memory location to become non-zero
#define await_signal(sig) _await_signal(WaitSignal, sig, Timeout::Infinite)
//! Waits for the byte at the specified memory location to become non-zero with the specified timeout
#define await_signal_timeout(sig, timeout) _await_signal(WaitSignal, sig, timeout)
//! Waits for the byte at the specified memory location to become non-zero until the specified instant
#define await_signal_until(sig, until) _await_signal(WaitSignal, sig, Timeout::Absolute(until))
//! Waits for the byte at the specified memory location to become non-zero for the specified number of milliseconds
#define await_signal_ms(sig, ms) _await_signal(WaitSignal, sig, Timeout::Milliseconds(ms))
//! Waits for the byte at the specified memory location to become non-zero for the specified number of seconds
#define await_signal_sec(sig, sec) _await_signal(WaitSignal, sig, Timeout::Seconds(sec))
//! Waits for the byte at the specified memory location to become non-zero for the specified number of platform-dependent ticks
#define await_signal_ticks(sig, ticks) _await_signal(WaitSignal, sig, Timeout::Ticks(ticks))

//! Waits indefinitely for the byte at the specified memory location to become zero
#define await_signal_off(sig) _await_signal(WaitInvertedSignal, sig, Timeout::Infinite)
//! Waits for the byte at the specified memory location to become zero with the specified timeout
#define await_signal_off_timeout(sig, timeout) _await_signal(WaitInvertedSignal, sig, timeout)
//! Waits for the byte at the specified memory location to become zero until the specified instant
#define await_signal_off_until(sig, until) _await_signal(WaitInvertedSignal, sig, Timeout::Absolute(until))
//! Waits for the byte at the specified memory location to become zero for the specified number of milliseconds
#define await_signal_off_ms(sig, ms) _await_signal(WaitInvertedSignal, sig, Timeout::Milliseconds(ms))
//! Waits for the byte at the specified memory location to become zero for the specified number of seconds
#define await_signal_off_sec(sig, sec) _await_signal(WaitInvertedSignal, sig, Timeout::Seconds(sec))
//! Waits for the byte at the specified memory location to become zero for the specified number of platform-dependent ticks
#define await_signal_off_ticks(sig, ticks) _await_signal(WaitInvertedSignal, sig, Timeout::Ticks(ticks))

//! Waits indefinitely for the acquisition of the specified bits
#define await_acquire(reg, mask)   _await_mask(WaitAcquire, reg, mask, 0, Timeout::Infinite)
//! Waits for the acquisition of the specified bits with the specified timeout
#define await_acquire_timeout(reg, mask, timeout) _await_mask(WaitAcquire, reg, mask, 0, timeout)
//! Waits for the acquisition of the specified bits until the specified instant
#define await_acquire_until(reg, mask, until) _await_mask(WaitAcquire, reg, mask, 0, Timeout::Absolute(until))
//! Waits for the acquisition of the specified bits for the specified number of milliseconds
#define await_acquire_ms(reg, mask, ms) _await_mask(WaitAcquire, reg, mask, 0, Timeout::Milliseconds(ms))
//! Waits for the acquisition of the specified bits for the specified number of seconds
#define await_acquire_sec(reg, mask, sec) _await_mask(WaitAcquire, reg, mask, 0, Timeout::Seconds(sec))
//! Waits for the acquisition of the specified bits for the specified number of platform-dependent ticks
#define await_acquire_ticks(reg, mask, ticks) _await_mask(WaitAcquire, reg, mask, 0, Timeout::Ticks(ticks))

//! Waits indefinitely for the inverse acquisition of the specified bits
#define await_acquire_zero(reg, mask)   _await_mask(WaitAcquire, reg, mask, mask, Timeout::Infinite)
//! Waits for the inverse acquisition of the specified bits with the specified timeout
#define await_acquire_zero_timeout(reg, mask, timeout) _await_mask(WaitAcquire, reg, mask, mask, timeout)
//! Waits for the inverse acquisition of the specified bits until the specified instant
#define await_acquire_zero_until(reg, mask, until) _await_mask(WaitAcquire, reg, mask, mask, Timeout::Absolute(until))
//! Waits for the inverse acquisition of the specified bits for the specified number of milliseconds
#define await_acquire_zero_ms(reg, mask, ms) _await_mask(WaitAcquire, reg, mask, mask, Timeout::Milliseconds(ms))
//! Waits for the inverse acquisition of the specified bits for the specified number of seconds
#define await_acquire_zero_sec(reg, mask, sec) _await_mask(WaitAcquire, reg, mask, mask, Timeout::Seconds(sec))
//! Waits for the inverse acquisition of the specified bits for the specified number of platform-dependent ticks
#define await_acquire_zero_ticks(reg, mask, ticks) _await_mask(WaitAcquire, reg, mask, mask, Timeout::Ticks(ticks))

//! Calls another async function
#define await(fn, ...) ({ \
    __label__ next; \
    f.__requireContinue(&&next); \
next: \
    auto res = fn(&__async.callee, ## __VA_ARGS__); \
    if (_ASYNC_RES_TYPE(res) != AsyncResult::Complete) return res; \
    _ASYNC_RES_VALUE(res); })

//! Spawns multiple tasks and awaits completion of all
#define await_all(...) ({ \
    __label__ next; \
    f.__requireContinue(&&next); \
    return ::kernel::Task::_RunAll(__async, __VA_ARGS__); \
next: __async.waitResult; })

//! Begins a block where multiple tasks can be spawned dynamically and then awaited
#define await_multiple_init() ({ \
    __async.children = 0; \
})

//! Adds a task to the group that will be awaited at once
#define await_multiple_add(...) ({ \
    __async.children++; \
    ::kernel::Task::Run(__VA_ARGS__).OnComplete(GetDelegate(&__async, &AsyncFrame::_child_completed)); \
})

//! Waits for all the tasks added using await_multiple_add() to complete
#define await_multiple() ({ \
    __label__ next; \
    f.__requireContinue(&&next); \
    return _ASYNC_RES(&__async, AsyncResult::WaitMultiple); \
next: __async.waitResult; })

//! Alias for @ref Delegate to an asynchronous function
template<typename... Args> using AsyncDelegate = Delegate<async_res_t, AsyncFrame**, Args...>;

//! Pointer to an asynchronous function
typedef async_res_t (*async_fptr_t)(AsyncFrame**);
//! Pointer to an asynchronous function with arguments
template<typename... Args> using async_fptr_args_t = async_res_t (*)(AsyncFrame**, Args... args);
//! Pointer to an asynchronous instance method
template<class T, typename... Args> using async_methodptr_t = async_res_t (T::*)(AsyncFrame**, Args... args);
