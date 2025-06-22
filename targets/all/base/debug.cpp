/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * debug.cpp
 */

#include <base/base.h>

#include <base/format.h>

#if TRACE && !(defined(PLATFORM_DBG_CHAR) && defined(PLATFORM_DBG_ACTIVE))
#warning "Disabling TRACE, as platform doesn't provide PLATFORM_DBG_ACTIVE and/or PLATFORM_DBG_CHAR"
#define PLATFORM_DBG_ACTIVE(ch) false
#define PLATFORM_DBG_CHAR(...)
#endif

#ifndef PLATFORM_DBG_BRACKET
#define PLATFORM_DBG_BRACKET()  '['
#endif

#if TRACE || MINITRACE

void DBG_PutChar(char ch)
{
    PLATFORM_DBG_CHAR(0, ch);
}

void DBG_PutString(const char* s)
{
    if (PLATFORM_DBG_ACTIVE(0))
    {
        while (auto c = *s++)
        {
            PLATFORM_DBG_CHAR(0, c);
        }
    }
}

#endif

#if TRACE

void DBG_AssertFailed(const char* file, unsigned line)
{
    DBG("ASSERT FAILED: %s(%u)\n", file, line);
    for (;;)
    {
#if DEBUG && defined(PLATFORM_WATCHDOG_HIT)
        volatile bool cont = false;
        PLATFORM_WATCHDOG_HIT();
        if (cont)
        {
            break;
        }
#endif
    }
}

void DBG_PrintF(const char* format, ...)
{
    if (PLATFORM_DBG_ACTIVE(0))
        va_call(vformat, format, (format_output)CDBG_PutChar, 0, format);
}

void DBG_PrintFV(const char* format, va_list va)
{
    if (PLATFORM_DBG_ACTIVE(0))
        vformat((format_output)CDBG_PutChar, 0, format, va);
}

void DBG_DebugPrintF(const char* format, ...)
{
    va_call_void(DebugPrintFV, format, 0, NULL, format);
}

void DBG_DebugPrintFC(const char* component, const char* format, ...)
{
    va_call_void(DebugPrintFV, format, 0, component, format);
}

OPTIMIZE void CDBG_PutChar(unsigned channel, char ch)
{
    PLATFORM_DBG_CHAR(channel, ch);
}

void CDBG_PrintF(unsigned channel, const char* format, ...)
{
    if (PLATFORM_DBG_ACTIVE(channel))
        va_call(vformat, format, (format_output)CDBG_PutChar, (void*)(uintptr_t)channel, format);
}

void CDBG_DebugPrintF(unsigned channel, const char* format, ...)
{
    va_call_void(DebugPrintFV, format, channel, NULL, format);
}

void CDBG_DebugPrintFC(unsigned channel, const char* component, const char* format, ...)
{
    va_call_void(DebugPrintFV, format, channel, component, format);
}

void DebugPrintFV(unsigned channel, const char* component, const char* format, va_list va)
{
    if (!PLATFORM_DBG_ACTIVE(channel))
        return;

    PLATFORM_CRITICAL_SECTION();    // prevent breaking Worker log output

#ifdef MONO_CLOCKS
    static struct
    {
        mono_t stamp, sec, sub;
    } last = {};

    mono_t stamp = MONO_CLOCKS;
    mono_t elapsed = last.stamp ? stamp - last.stamp : 0;
    last.stamp = stamp;

    mono_t sub = last.sub + elapsed;
    mono_t sec = sub / MONO_FREQUENCY;
    last.sub = sub -= sec * MONO_FREQUENCY;
    last.sec = sec += last.sec;
    sub = sub * (((uint64_t)1000000 << 32) / MONO_FREQUENCY) >> 32;  // scale to microseconds

    char bracket = PLATFORM_DBG_BRACKET();
    bool nz = false;

    PLATFORM_DBG_CHAR(channel, bracket);
    for (int n = 1000000000; n; n /= 10)
    {
        int c = sec / n;
        sec -= c * n;
        if (c || nz)
        {
            PLATFORM_DBG_CHAR(channel, '0' + c);
            nz = true;
        }
        else if (c || n <= 1000)
        {
            PLATFORM_DBG_CHAR(channel, c ? '0' + c : ' ');
        }
    }

    for (int n = 100000; n; n /= 10)
    {
        int c = sub / n;
        sub -= c * n;
        if (c || n == 1000)
            nz = true;
        PLATFORM_DBG_CHAR(channel, c || nz ? '0' + c : ' ');
        if (n == 1000)
            PLATFORM_DBG_CHAR(channel, '.');
    }

    PLATFORM_DBG_CHAR(channel, bracket == '(' ? ')' : bracket + 2);
    PLATFORM_DBG_CHAR(channel, ' ');
#endif

    if (component)
    {
        while (*component)
            PLATFORM_DBG_CHAR(channel, *component++);
        PLATFORM_DBG_CHAR(channel, ':');
        PLATFORM_DBG_CHAR(channel, ' ');
    }

    vformat((format_output)CDBG_PutChar, (void*)(uintptr_t)channel, format, va);
}

#endif
