/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/Timeout.cpp
 */

#include "Timeout.h"

bool Timeout::Pending(mono_t at)
{
    if (!value) { return false; }
    if (IsRelative()) { value = (at + value) | (mono_t(MONO_SIGNED_MAX) + 1); }
    if (Relative(at) < 0) { value = 0; return false; }
    return true;
}
