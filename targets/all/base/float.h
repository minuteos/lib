/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * base/float.h
 *
 * Fast roundtrip-safe FP32 to string and back conversion routines
 */

#pragma once

#include <base/base.h>

/*!
 * Converts the specified float value to a string representation
 * @param v Single-precision value to convert to a string
 * @param buf Output buffer, at most 15 characters are generated
 * @returns Pointer to the end of generated characters, NUL is not written
 */
EXTERN_C char* fast_ftoa(float v, char* buf);

/*!
 * Converts the specified string to a single-precision float
 * @param s String to convert to a floating-point value
 * @returns Best-effort conversion of the string value, or NAN if it conversion wasn't possible
 */
EXTERN_C float fast_atof(const char* s);

/*!
 * Creates a single-precision float value from the specified inputs
 * @param sign Sign of the result (explicit because float supports negative zero etc.)
 * @param significand Significand (mantissa) of the result
 * @param exponent Decimal exponent of the result
 * @returns A single-precision float as close as possible to the input parameters
 */
EXTERN_C float fast_itof(bool sign, uint32_t significand, int exponent);
