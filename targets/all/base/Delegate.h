/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * Delegate.h
 *
 * Strongly-typed pointer to member function combined with target. Virtual
 * functions are resolved ahead of time.
 */

#pragma once

#include <base/base.h>

#include <base/Packed.h>

#if defined(__arm__) || defined(__mips__) || defined(__aarch64__)
// for architectures where function pointers can be odd, the virtual indicator is the lowest bit of thisAdjust field
#define PMF_ODD_FPTR	1
#else
// for architectures where function pointers are always even, the virtual indicator is the lowest bit of fptrOrVtOffset
#define PMF_ODD_FPTR	0
#endif

struct _PMFInternal
{
    ptrdiff_t fptrOrVtOffset, thisAdjust;
};

struct _PMFDecodeResult
{
    typedef void (*fptr_t)(void*);

    void* target;
    fptr_t fptr;
};

Packed<_PMFDecodeResult> _PMFDecode(void* target, ptrdiff_t fptrOrVtOffset, ptrdiff_t thisAdjust);

using _Delegate = Packed<_PMFDecodeResult>;

ALWAYS_INLINE void* _DelegateTarget(_Delegate packed) { return unpack<_PMFDecodeResult>(packed).target; }
ALWAYS_INLINE _PMFDecodeResult::fptr_t _DelegateFn(_Delegate packed) { return unpack<_PMFDecodeResult>(packed).fptr; }

template<typename TRes, typename... Args>
class Delegate
{
public:
    typedef TRes (*fptr_t)(void* target, Args...);
    template<typename T> using tfptr_t = TRes (*)(T* target, Args...);
    template<typename T> using mthptr_t = TRes (T::*)(Args...);

private:
    union
    {
        struct
        {
            void* target;
            fptr_t fn;
        };
        _Delegate packed;
    };

    template<class T, typename TRes2, typename... Args2> friend constexpr Delegate<TRes2, Args2...> GetDelegate(T* target, TRes (T::*method)(Args...));

    ALWAYS_INLINE constexpr void Init(void* target, _PMFInternal& rep)
    {
        // try to inline decoding of non-virtual functions if the compiler knows ahead of time if the function is virtual or not
#if PMF_ODD_FPTR
        if (__builtin_constant_p(rep.thisAdjust) && !(rep.thisAdjust & 1))
#else
        if (__builtin_constant_p(rep.fptrOrVtOffset) && !(rep.fptrOrVtOffset & 1))
#endif
        {
            this->target = (void*)(uintptr_t(target) + (rep.thisAdjust >> PMF_ODD_FPTR));
            this->fn = (fptr_t)rep.fptrOrVtOffset;
        }
        else
        {
            this->packed = _PMFDecode(target, rep.fptrOrVtOffset, rep.thisAdjust);
        }
    }

public:
    ALWAYS_INLINE constexpr Delegate(fptr_t fn = NULL, void* arg0 = NULL) : target(arg0), fn(fn) {}

    template<class T>
    ALWAYS_INLINE constexpr Delegate(tfptr_t<T> fn, T* arg0 = NULL) : target(arg0), fn((fptr_t)fn) {}

    template<class T>
    ALWAYS_INLINE constexpr Delegate(T* target, TRes (T::*method)(Args...))
    {
        Init(target, unsafe_cast<_PMFInternal>(method));
    }

    template<class T>
    ALWAYS_INLINE constexpr Delegate(const T* target, TRes (T::*method)(Args...) const)
    {
        Init(target, unsafe_cast<_PMFInternal>(method));
    }

    ALWAYS_INLINE constexpr TRes operator()(Args... args) { return fn(target, args...); }

    ALWAYS_INLINE constexpr operator bool() const { return fn; }

    ALWAYS_INLINE constexpr operator _Delegate() const { return packed; }

    ALWAYS_INLINE constexpr bool operator ==(_Delegate other) const { return this->packed == packed; }
};

template<class T, typename TRes, typename... Args>
ALWAYS_INLINE constexpr Delegate<TRes, Args...> GetDelegate(TRes (*fn)(T* a0, Args...), T* arg0 = NULL)
{
    return Delegate<TRes, Args...>(fn, arg0);
}

template<class T, typename TRes, typename... Args>
ALWAYS_INLINE constexpr Delegate<TRes, Args...> GetDelegate(T* target, TRes (T::*method)(Args...))
{
    return Delegate<TRes, Args...>(target, method);
}

template<class T, typename TRes, typename... Args>
ALWAYS_INLINE constexpr Delegate<TRes, Args...> GetDelegate(const T* target, TRes (T::*method)(Args...) const)
{
    return Delegate<TRes, Args...>(target, method);
}
