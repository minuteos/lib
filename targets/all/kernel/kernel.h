/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel/kernel.h
 *
 * Primary header for the kernel component
 */

#pragma once

#include <kernel/config.h>
#include <kernel/async.h>
#include <kernel/mono.h>
#include <kernel/atomic.h>

#include <kernel/Scheduler.h>
#include <kernel/Task.h>

#ifdef Ckernel_app
#include <kernel-app/kernel.h>
#endif
