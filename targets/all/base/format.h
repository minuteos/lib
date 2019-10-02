/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * format.h
 *
 * Lightweight printf-style formatting
 */

#pragma once

#include <base/base.h>

struct format_write_info
{
    char* p;
    char* end;
};

typedef void (*format_output)(void* context, char ch);

BEGIN_EXTERN_C

extern void format_output_discard(void* context, char ch);
extern void format_output_mem(void* write_info, char ch);
extern void format_output_unsafe(void* pp, char ch);

extern int format(format_output output, void* context, const char* format, ...);
extern int vformat(format_output output, void* context, const char* format, va_list va);

END_EXTERN_C
