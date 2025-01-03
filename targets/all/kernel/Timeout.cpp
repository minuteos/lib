/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/Timeout.cpp
 */

#include "Timeout.h"

Timeout Timeout::MakeAbsoluteImpl(mono_t value)
{
    if (value == 0 || value > MONO_SIGNED_MAX) { return value; }
    return MakeAbsoluteImpl(value, MONO_CLOCKS);
}

Timeout Timeout::MakeAbsoluteImpl(mono_t value, mono_t relativeTo)
{
    if (value == 0 || value > MONO_SIGNED_MAX) { return value; }
    return (relativeTo + value) | (mono_t(MONO_SIGNED_MAX) + 1);
}

bool Timeout::Pending()
{
    return Pending(MONO_CLOCKS);
}

bool Timeout::Pending(mono_t at)
{
    if (!value) { return false; }
    if (IsRelative()) { value = (at + value) | (mono_t(MONO_SIGNED_MAX) + 1); }
    if (Relative(at) < 0) { value = 0; return false; }
    return true;
}
