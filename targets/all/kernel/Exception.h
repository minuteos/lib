/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/Exception.h
 */

#pragma once

#include <base/base.h>

#define DECLARE_EXCEPTION(name) \
    extern const ::kernel::ExceptionTypeDefinition name ## _; \
    static constexpr ::kernel::ExceptionType name = ::kernel::ExceptionType(name ## _);

#define DEFINE_EXCEPTION(name) \
    const ::kernel::ExceptionTypeDefinition name ## _ = { #name }; \

namespace kernel
{

struct ExceptionTypeDefinition
{
    const char* name;
};

class ExceptionType
{
private:
    const ExceptionTypeDefinition* def;

public:
    ALWAYS_INLINE constexpr ExceptionType(const ExceptionTypeDefinition& def)
        : def(&def) {}
    ALWAYS_INLINE constexpr ExceptionType(AsyncResult result)
        : def(result == AsyncResult::Complete ? NULL : (ExceptionTypeDefinition*)(~uintptr_t(result) << 1)) {}

    ALWAYS_INLINE constexpr operator AsyncResult() const
    {
        return def ? (AsyncResult)(~(uintptr_t(def) >> 1)) : AsyncResult::Complete;
    }

    ALWAYS_INLINE constexpr operator const ExceptionTypeDefinition*() const { return def; }
    ALWAYS_INLINE constexpr const char* Name() const { return def ? def->name : NULL; }
    ALWAYS_INLINE constexpr bool operator ==(const ExceptionType& other) { return def == other.def; }
};

DECLARE_EXCEPTION(Error);

class Exception
{
public:
    constexpr Exception(ExceptionType type, intptr_t value)
        : type(type), value(value) {}

    operator bool() const { return type; }
    ExceptionType Type() const { return type; }
    const char* Name() const { return type.Name(); }
    intptr_t Value() const { return value; }

private:
    ExceptionType type;
    intptr_t value;
};

}
