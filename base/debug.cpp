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

#if TRACE

void DBG_AssertFailed(const char* file, unsigned line)
{
	DBG("ASSERT FAILED: %s(%u)\n", file, line);
	for (;;);
}

void CDBG_PutCharDirect(unsigned channel, char ch)
{
	PLATFORM_DBG_CHAR(channel, ch);
}

void DBG_PutChar(char ch)
{
	if (PLATFORM_DBG_ACTIVE(0))
		PLATFORM_DBG_CHAR(0, ch);
}

void DBG_PrintF(const char* format, ...)
{
	if (PLATFORM_DBG_ACTIVE(0))
		va_call(vformat, format, (format_output)CDBG_PutCharDirect, 0, format);
}

void DBG_DebugPrintF(const char* format, ...)
{
	va_call_void(DebugPrintFV, format, 0, NULL, format);
}

void DBG_DebugPrintFC(const char* component, const char* format, ...)
{
	va_call_void(DebugPrintFV, format, 0, component, format);
}

void CDBG_PutChar(unsigned channel, char ch)
{
	if (PLATFORM_DBG_ACTIVE(channel))
		PLATFORM_DBG_CHAR(channel, ch);
}

void CDBG_PrintF(unsigned channel, const char* format, ...)
{
	if (PLATFORM_DBG_ACTIVE(channel))
		va_call(vformat, format, (format_output)CDBG_PutCharDirect, (void*)(uintptr_t)channel, format);
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

#ifdef MONO_US
	static uint s_last = MONO_US, s_sec, s_sub;

	uint elapsed = MONO_US - s_last;
	s_last += elapsed;

	int sub = s_sub + elapsed;
	int sec = sub / 1000000;
	s_sub = sub -= sec * 1000000;
	s_sec = sec += s_sec;

	bool nz = false;

	PLATFORM_DBG_CHAR(channel, '[');
	for (uint n = 1000000000; n; n /= 10)
	{
		uint c = sec / n;
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

	for (uint n = 100000; n; n /= 10)
	{
		uint c = sub / n;
		sub -= c * n;
		if (c || n == 1000)
			nz = true;
		PLATFORM_DBG_CHAR(channel, c || nz ? '0' + c : ' ');
		if (n == 1000)
			PLATFORM_DBG_CHAR(channel, '.');
	}

	PLATFORM_DBG_CHAR(channel, ']');
	PLATFORM_DBG_CHAR(channel, ' ');
#endif

	if (component)
	{
		while (*component)
			PLATFORM_DBG_CHAR(channel, *component++);
		PLATFORM_DBG_CHAR(channel, ':');
		PLATFORM_DBG_CHAR(channel, ' ');
	}

	vformat((format_output)CDBG_PutCharDirect, (void*)(uintptr_t)channel, format, va);
}

#endif
