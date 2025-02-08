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
static uint32_t _tcStart, _tcTotal;

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

    printf("<details>\n\n");
    printf("| Test file | Test name | Duration (ms) | Result |\n");
    printf("| :-------- | :-------- | ------------: | :----: |\n");

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

    printf("| **TOTAL** | **%d** | **%u.%03u** | **%d** :white_check_mark: / **%d** :x: |\n", passed + failed,
        unsigned(_tcTotal / 1000), unsigned(_tcTotal % 1000),
        passed, failed);
    printf("<summary>%d tests (<b>%s</b>)\n\n", passed + failed, failed ? "FAILED" : "PASSED");
    printf("**%d** :white_check_mark: / **%d** :x:\n", passed, failed);
    printf("</summary></details>\n");
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
    _tcStart = MONO_US;
    Run();
    PrintResult();
    return false;
}

#if Ckernel
void TestCase::_AssertException(int line, AsyncCatchResult result, ::kernel::ExceptionType type, intptr_t value)
{
    _AssertEqual(line, result.ExceptionType(), type);
    _AssertEqual(line, result.Value(), value);
}
#endif

void TestCase::_Fail(int line)
{
    _Fail(line, std::function<void(void)>{});
}

void TestCase::_Fail(int line, const char* reason)
{
    _Fail(line, [reason] { printf("%s", reason); });
}

void TestCase::_Fail(int line, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    _Fail(line, [format, &va] { vprintf(format, va); });
    va_end(va);
}

void TestCase::_Fail(int line, std::function<void(void)> reason)
{
    s_cur->PrintResult(line, reason);
#if Ckernel
    ::kernel::Scheduler::Main().Reset();
#endif
    longjmp(*s_failJump, 1);
}

void TestCase::PrintResult(int errorLine, std::function<void(void)> errorReason)
{
    auto dur = MONO_US - _tcStart;
    _tcTotal += dur;
    printf("| %s:%d ", File(), Line());
    if (errorLine)
    {
       printf("<br/> *...assertion at line %d* ", errorLine);
    }
    printf("| %s | %u.%03u | %s ", Name(), unsigned(dur / 1000), unsigned(dur % 1000), errorLine ? ":x:" : ":white_check_mark:");
    if (errorReason)
    {
        printf("<br/> **(");
        errorReason();
        puts(")** |");
    }
    else
    {
        puts("|");
    }
}

extern "C" void TEST_AssertFailed(const char* file, unsigned line)
{
    TestCase::_Fail(line, "ASSERT: %s:%u", file, line);
}
