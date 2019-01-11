/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 * 
 * base.h - primary header for the base component
 */

#pragma once

#include <defs.h>

// include a few common system headers
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <base/macros.h>
#include <base/hacks.h>
#include <base/helpers.h>
#include <base/bitfields.h>
#include <base/enums.h>
#include <base/debug.h>

// platform-specific base header, to include anything that is common for the platform
// e.g. CMSIS for ARM targets
#include <base/platform.h>
