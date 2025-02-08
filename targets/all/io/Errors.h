/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/TimeoutError.h
 */

#pragma once

#include <kernel/kernel.h>

namespace io
{

DECLARE_EXCEPTION(TimeoutError);
DECLARE_EXCEPTION(AbortError);

}
