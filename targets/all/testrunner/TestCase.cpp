/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 */

#include <testrunner/TestCase.h>

#ifdef Ckernel
#include <kernel/kernel.h>
#endif

TestCase* TestCase::s_first = NULL;
TestCase* TestCase::s_last = NULL;
TestCase* TestCase::s_cur = NULL;
jmp_buf* TestCase::s_failJump = NULL;

TestCase::TestCase()
{
    if (s_last)
        s_last->next = this;
    else
        s_first = this;
    s_last = this;
}

bool TestCase::RunAll(int argc, char** argv)
{
    int passed = 0, failed = 0;

    for (auto tc = s_first; tc; tc = tc->next)
    {
        if (!tc->Match(argc, argv))
        {
            continue;
        }

        if (tc->Execute())
            failed++;
        else
            passed++;
    }

    printf("\nRan %d tests, %d passed, %d failed\n", passed + failed, passed, failed);
    return failed != 0;
}

bool TestCase::Match(int numFilters, char** filters)
{
    if (!numFilters)
    {
        return true;
    }

    auto name = Name();
    auto nameLen = strlen(name);

    for (int i = 0; i < numFilters; i++)
    {
        auto f = filters[i];
        bool anyPrefix = f[0] == '*';
        if (anyPrefix) { f++; }
        auto len = strlen(f);
        bool anySuffix = len && f[len - 1] == '*';
        if (anySuffix) { len--; }

        if (len > nameLen)
        {
            // if filter length is more than name length, it can never match
            continue;
        }

        if (!anyPrefix && !anySuffix)
        {
            if (len == nameLen && !strcasecmp(name, f))
            {
                return true;
            }
        }
        else if (!anyPrefix)
        {
            // prefix match
            if (!strncasecmp(name, f, len))
            {
                return true;
            }
        }
        else if (!anySuffix)
        {
            // suffix match
            if (!strncasecmp(name + len - nameLen, f, len))
            {
                return true;
            }
        }
        else
        {
            // any match
            for (size_t i = 0; i < nameLen - len; i++)
            {
                if (!strncasecmp(name + i, f, len))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool TestCase::Execute()
{
    jmp_buf fail;
    printf("%s:%d - %s...", File(), Line(), Name());
    if (setjmp(fail))
    {
        // we get here after a failure
        return true;
    }

    s_failJump = &fail;
    s_cur = this;
#ifdef Ckernel
    __testrunner_time = 0;
#endif
    Run();
    puts("OK");
    return false;
}

void TestCase::_Fail(int line)
{
    puts("FAIL");
    printf("  assertion at %s:%d\n", s_cur->File(), line);
    longjmp(*s_failJump, 1);
}

void TestCase::_Fail(int line, const char* reason)
{
    printf("FAIL (%s)\n", reason);
    printf("  assertion at %s:%d\n", s_cur->File(), line);
    longjmp(*s_failJump, 1);
}

void TestCase::_Fail(int line, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    printf("FAIL (");
    vprintf(format, va);
    puts(")");
    va_end(va);
    printf("  assertion at %s:%d\n", s_cur->File(), line);
    longjmp(*s_failJump, 1);
}

void TestCase::_Fail(int line, std::function<void(void)> reason)
{
    printf("FAIL (");
    reason();
    puts(")");
    printf("  assertion at %s:%d\n", s_cur->File(), line);
    longjmp(*s_failJump, 1);
}
