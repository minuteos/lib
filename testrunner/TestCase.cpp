/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 */

#include <testrunner/TestCase.h>

TestCase* TestCase::s_first = NULL;
TestCase* TestCase::s_last = NULL;

TestCase::TestCase()
{
    if (s_last)
        s_last->next = this;
    else
        s_first = this;
    s_last = this;
}

bool TestCase::RunAll()
{
    int passed = 0, failed = 0;

    for (auto tc = s_first; tc; tc = tc->next)
    {
        if (tc->Execute())
            failed++;
        else
            passed++;
    }

    printf("\nRan %d tests, %d passed, %d failed\n", passed + failed, passed, failed);
    return failed != 0;
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
    
    failJump = &fail;
    Run();
    puts("OK");
    return false;
}

void TestCase::Fail()
{
    puts("FAIL");
    longjmp(*failJump, 1);
}

void TestCase::Fail(const char* reason)
{
    printf("FAIL (%s)\n", reason);
    longjmp(*failJump, 1);
}

void TestCase::Fail(const char* format, ...)
{   
    va_list va;
    va_start(va, format);
    printf("FAIL (");
    vprintf(format, va);
    puts(")");
    va_end(va);
    longjmp(*failJump, 1);
}

void TestCase::Fail(std::function<void(void)> reason)
{
    printf("FAIL (");
    reason();
    puts(")");
    longjmp(*failJump, 1);
}
