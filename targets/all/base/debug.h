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

extern void DBG_PutChar(char ch);
extern void DBG_PutString(const char* s);
extern void DBG_PrintF(const char* format, ...);
extern void DBG_PrintFV(const char* format, va_list va);
extern void DBG_DebugPrintF(const char* format, ...);
extern void DBG_DebugPrintFC(const char* component, const char* format, ...);
extern void CDBG_PutChar(unsigned channel, char ch);
extern void CDBG_PrintF(unsigned channel, const char* format, ...);
extern void CDBG_DebugPrintF(unsigned channel, const char* format, ...);
extern void CDBG_DebugPrintFC(unsigned channel, const char* component, const char* format, ...);
extern void DebugPrintFV(unsigned channel, const char* component, const char* format, va_list va);

#if DEBUG || TRACE_ASSERT

extern void DBG_AssertFailed(const char* file, unsigned line);
#define ASSERT(expr)	 if(!(expr)) { DBG_AssertFailed(__FILE__, __LINE__); }

#else

#define ASSERT(expr)

#endif

#define DBG(...)                    DBG_DebugPrintF(__VA_ARGS__)
#define DBGC(component, ...)        DBG_DebugPrintFC(component, __VA_ARGS__)
#define DBGL(fmt, ...)              DBG_DebugPrintF(fmt "\n", ## __VA_ARGS__)
#define DBGCL(component, fmt, ...)  DBG_DebugPrintFC(component, fmt "\n", ## __VA_ARGS__)
#define DBGS(s)                     DBG_DebugPrintF("%s", s)
#define _DBG(...)                   DBG_PrintF(__VA_ARGS__)
#define _DBGVA(...)                 DBG_PrintFV(__VA_ARGS__)
#define _DBGCHAR(c)                 DBG_PutChar(c)
#define _DBGS(s)                    DBG_PutString(s)

#define CDBG(ch, ...)               CDBG_DebugPrintF(ch, __VA_ARGS__)
#define CDBGC(ch, component, ...)   CDBG_DebugPrintFC(ch, component, __VA_ARGS__)
#define _CDBG(ch, ...)              CDBG_PrintF(ch, __VA_ARGS__)
#define _CDBGCHAR(ch, c)            CDBG_PutChar(ch, c)

#define CDBG8(ch, c)                PLATFORM_DBG_CHAR(ch, c)
#define CDBG16(ch, h)               PLATFORM_DBG_HALFWORD(ch, h)
#define CDBG32(ch, w)               PLATFORM_DBG_WORD(ch, unsafe_cast<uint32_t>(w))

#else

#if MINITRACE
extern void DBG_PutChar(char ch);
extern void DBG_PutString(const char* s);
#endif

#define ASSERT(expr)

#define DBG(...)
#define DBGC(component, ...)
#define DBGL(fmt, ...)
#define DBGCL(component, fmt, ...)
#define _DBG(...)
#define _DBGVA(...)
#if MINITRACE
#define _DBGCHAR(c)                 DBG_PutChar(c)
#define DBGS(s)                     ({ DBG_PutString(s); DBG_PutChar('\n'); })
#define _DBGS(s)                    DBG_PutString(s)
#else
#define _DBGCHAR(c)
#define DBGS(...)
#define _DBGS(...)
#endif

#define CDBG(ch, ...)
#define CDBGC(ch, component, ...)
#define _CDBG(ch, ...)
#define _CDBGCHAR(ch, c)

#define CDBG8(ch, h)
#define CDBG16(ch, h)
#define CDBG32(ch, h)

#endif

#if Ctestrunner

extern void TEST_AssertFailed(const char* file, unsigned line);

#undef ASSERT
#define ASSERT(expr)    if(!(expr)) { TEST_AssertFailed(__FILE__, __LINE__); }

#endif

END_EXTERN_C
