/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * debug.h
 */


#pragma once

#include <base/base.h>

#ifndef DEBUG
#define DEBUG	0
#endif

#ifndef TRACE
#define TRACE   DEBUG
#endif

BEGIN_EXTERN_C

#if TRACE

extern void DBG_AssertFailed(const char* file, unsigned line);

extern void DBG_PutChar(char ch);
extern void DBG_PrintF(const char* format, ...);
extern void DBG_DebugPrintF(const char* format, ...);
extern void DBG_DebugPrintFC(const char* component, const char* format, ...);
extern void CDBG_PutChar(unsigned channel, char ch);
extern void CDBG_PrintF(unsigned channel, const char* format, ...);
extern void CDBG_DebugPrintF(unsigned channel, const char* format, ...);
extern void CDBG_DebugPrintFC(unsigned channel, const char* component, const char* format, ...);
extern void DebugPrintFV(unsigned channel, const char* component, const char* format, va_list va);

#define ASSERT(expr)	 if(!(expr)) { DBG_AssertFailed(__FILE__, __LINE__); }

#define DBG(...)                    DBG_DebugPrintF(__VA_ARGS__)
#define DBGC(component, ...)        DBG_DebugPrintFC(component, __VA_ARGS__)
#define DBGL(fmt, ...)              DBG_DebugPrintF(fmt "\n", ## __VA_ARGS__)
#define DBGCL(component, fmt, ...)  DBG_DebugPrintFC(component, fmt "\n", ## __VA_ARGS__)
#define _DBG(...)                   DBG_PrintF(__VA_ARGS__)
#define _DBGCHAR(c)                 DBG_PutChar(c)

#define CDBG(ch, ...)               CDBG_DebugPrintF(ch, __VA_ARGS__)
#define CDBGC(ch, component, ...)   CDBG_DebugPrintFC(ch, component, __VA_ARGS__)
#define _CDBG(ch, ...)              CDBG_PrintF(ch, __VA_ARGS__)
#define _CDBGCHAR(ch, c)            CDBG_PutChar(ch, c)

#else

#define ASSERT(expr)

#define DBG(...)
#define DBGC(component, ...)
#define DBGL(fmt, ...)
#define DBGCL(component, fmt, ...)
#define _DBG(...)
#define _DBGCHAR(c)

#define CDBG(ch, ...)
#define CDBGC(ch, component, ...)
#define _CDBG(ch, ...)
#define _CDBGCHAR(ch, c)

#endif

END_EXTERN_C
