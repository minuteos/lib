/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 */

#pragma once

#include <base/base.h>
#include <base/Span.h>

#include <setjmp.h>
#include <functional>

class TestCase
{
public:
    static bool RunAll(int numFilters = 0, char** filters = NULL);

    static TestCase* Active() { return s_cur; }

protected:
    TestCase();
    virtual void Run() = 0;
    virtual const char* Name() const = 0;
    virtual const char* File() const = 0;
    virtual int Line() const = 0;

public:
    static void _Assert(int line, bool condition) { if (!condition) _Fail(line); }
    template<typename T1, typename T2> static void _AssertEqual(int line, T1 o1, T2 o2) { if (!(o1 == o2)) _Fail(line, [o1,o2] { Print(o1); Print(" != "); Print(o2); }); }
    template<typename T1, typename T2> static void _AssertNotEqual(int line, T1 o1, T2 o2) { if (!(o1 != o2)) _Fail(line, [o1,o2] { Print(o1); Print(" == "); Print(o2); }); }
    template<typename T1, typename T2> static void _AssertLessThan(int line, T1 o1, T2 o2) { if (!(o1 < o2)) _Fail(line, [o1,o2] { Print(o1); Print(" >= "); Print(o2); }); }
    template<typename T1, typename T2> static void _AssertGreaterThan(int line, T1 o1, T2 o2) { if (!(o1 > o2)) _Fail(line, [o1,o2] { Print(o1); Print(" <= "); Print(o2); }); }
    template<typename T1, typename T2> static void _AssertLessOrEqual(int line, T1 o1, T2 o2) { if (!(o1 <= o2)) _Fail(line, [o1,o2] { Print(o1); Print(" > "); Print(o2); }); }
    template<typename T1, typename T2> static void _AssertGreaterOrEqual(int line, T1 o1, T2 o2) { if (!(o1 >= o2)) _Fail(line, [o1,o2] { Print(o1); Print(" < "); Print(o2); }); }
    static void _AssertEqualString(int line, const char* s1, const char* s2) { if (strcmp(s1, s2)) _Fail(line, [s1,s2] { Print(s1); Print(" != "); Print(s2); }); }
    static void _AssertNotEqualString(int line, const char* s1, const char* s2) { if (!strcmp(s1, s2)) _Fail(line, [s1,s2] { Print(s1); Print(" == "); Print(s2); }); }
    static void _Fail(int line);
    static void _Fail(int line, const char* reason);
    static void _Fail(int line, const char* format, ...);
    static void _Fail(int line, std::function<void(void)> reasonPrint);
    static void _Done() { s_cur = NULL; }

    static void Print(const char* text) { printf("%s", text); }
    static void Print(int value) { printf("%d", value); }
    static void Print(unsigned int value) { printf("%u", value); }
    static void Print(long value) { printf("%ld", value); }
    static void Print(unsigned long value) { printf("%lu", value); }
    static void Print(Span value) { for (char c: value) printf("%02X", c); }
    template<typename T> static void Print(const T* ptr) { printf("%p", ptr); }
    template<typename T> static void Print(T* ptr) { printf("%p", ptr); }
    // everything else is printed as raw data
    template<typename T> static void Print(T value) { Print(Span::Of(value)); }

private:
    bool Match(int numFilters, char** filters);
    bool Execute();
    void PrintResult(int errorLine = 0, std::function<void(void)> errorReason = {});

    static TestCase* s_first;
    static TestCase* s_last;
    static TestCase* s_cur;
    static jmp_buf* s_failJump;
    TestCase* next;
};

#define Assert(...) _Assert(__LINE__, ## __VA_ARGS__)
#define AssertEqual(...) _AssertEqual(__LINE__, ## __VA_ARGS__)
#define AssertNotEqual(...) _AssertNotEqual(__LINE__, ## __VA_ARGS__)
#define AssertLessThan(...) _AssertLessThan(__LINE__, ## __VA_ARGS__)
#define AssertGreaterThan(...) _AssertGreaterThan(__LINE__, ## __VA_ARGS__)
#define AssertLessOrEqual(...) _AssertLessOrEqual(__LINE__, ## __VA_ARGS__)
#define AssertGreaterOrEqual(...) _AssertGreaterOrEqual(__LINE__, ## __VA_ARGS__)
#define AssertEqualString(...) _AssertEqualString(__LINE__, ## __VA_ARGS__)
#define AssertNotEqualString(...) _AssertNotEqualString(__LINE__, ## __VA_ARGS__)
#define Fail(...) _Fail(__LINE__, ## __VA_ARGS__)

#define TEST_CASE(name) _TEST_CASE(name, __COUNTER__)
#define _TEST_CASE(name, id) __TEST_CASE(name, id)
#define __TEST_CASE(name, id) \
class TestCase_ ## id : public TestCase \
{ protected: virtual void Run() final override; \
  virtual const char* Name() const final override { return name; } \
  virtual const char* File() const final override { return __FILE__; } \
  virtual int Line() const final override { return __LINE__; } \
} _tc_ ## id; \
void TestCase_ ## id ::Run()

#define async_test  { struct __AsyncTest
#define async_test_init(...) __AsyncTest() : __VA_ARGS__ {}
#define async_test_end ; \
    struct __TestRunner { \
        __AsyncTest test; \
        async(Run) async_def() { await(test.Run); _Done(); } async_end \
    } test; \
    ::kernel::Scheduler::Main().Add(test, &__TestRunner::Run); \
    ::kernel::Scheduler::Main().Run(); \
}
