/*
 * Copyright (c) 2022 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/MemPoolAsync.h
 */

#pragma once

#include <base/base.h>

#if Ckernel
#include <kernel/kernel.h>

#define __mempool_waitptr(T) (*MemPoolGet<T>()->WatchPointer())

#define await_mempool_timeout(T, timeout) \
    await_mask_not_timeout(__mempool_waitptr(T), ~0u, __mempool_waitptr(T), timeout)
#define await_mempool_until(T, until) \
    await_mask_not_until(__mempool_waitptr(T), ~0u, __mempool_waitptr(T), until)
#define await_mempool_ms(T, ms) \
    await_mask_not_ms(__mempool_waitptr(T), ~0u, __mempool_waitptr(T), ms)
#define await_mempool_sec(T, sec) \
    await_mask_not_sec(__mempool_waitptr(T), ~0u, __mempool_waitptr(T), sec)
#define await_mempool_ticks(T, timeout) \
    await_mask_not_ticks(__mempool_waitptr(T), ~0u, __mempool_waitptr(T), ticks)

#endif
