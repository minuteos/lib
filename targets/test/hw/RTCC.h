/*
 * Copyright (c) 2022 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * test/hw/RTCC.h
 */

#pragma once

#include <base/base.h>

#define RTCC    ((_RTCC*)NULL)

class _RTCC
{
public:
    uint32_t Time() const { return MONO_CLOCKS / MONO_FREQUENCY; }
};
