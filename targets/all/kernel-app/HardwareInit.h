/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel-app/HardwareInit.h
 */

#pragma once

#include <kernel/kernel.h>

#ifndef HARDWARE_INIT_PRIORITY
#define HARDWARE_INIT_PRIORITY 1000
#endif

#define HARDWARE_INIT(...) INIT_FUNCTION_PRIO(HARDWARE_INIT_PRIORITY) static void UNIQUE(__hardwareInit)() { __VA_ARGS__(); }
