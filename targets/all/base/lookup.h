/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/lookup.h
 */

#pragma once

#include <base/base.h>

//! define a constant lookup table with elements of the specified type
#define LOOKUP_TABLE(type, ...)	((const type[]){ __VA_ARGS__ })

//! define a lookup table of 8-bit bytes
#define BYTES(...)	LOOKUP_TABLE(uint8_t, ## __VA_ARGS__)
//! define a lookup table of 32-bit words
#define WORDS(...)	LOOKUP_TABLE(uint32_t, ## __VA_ARGS__)
//! define a lookup table of null-terminated string literals
#define STRINGS(...)	LOOKUP_TABLE(char*, ## __VA_ARGS__)
