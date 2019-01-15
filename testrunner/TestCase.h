/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 */

#pragma once

#include <base/base.h>

#include <setjmp.h>
#include <functional>

class TestCase
{
public:
    static bool RunAll();

protected:
    TestCase();
    virtual void Run() = 0;
    virtual const char* Name() const = 0;
    virtual const char* File() const = 0;
    virtual int Line() const = 0;

public:
    void _Assert(int line, bool condition) { if (!condition) _Fail(line); }
    template<typename T1, typename T2> void _AssertEqual(int line, T1 o1, T2 o2) { if (o1 != o2) _Fail(line, [this,o1,o2] { Print(o1); Print(" != "); Print(o2); }); }
    template<typename T1, typename T2> void _AssertNotEqual(int line, T1 o1, T2 o2) { if (o1 == o2) _Fail(line, [this,o1,o2] { Print(o1); Print(" == "); Print(o2); }); }
    template<typename T1, typename T2> void _AssertLessThan(int line, T1 o1, T2 o2) { if (o1 >= o2) _Fail(line, [this,o1,o2] { Print(o1); Print(" >= "); Print(o2); }); }
    template<typename T1, typename T2> void _AssertGreaterThan(int line, T1 o1, T2 o2) { if (o1 <= o2) _Fail(line, [this,o1,o2] { Print(o1); Print(" <= "); Print(o2); }); }
    template<typename T1, typename T2> void _AssertLessOrEqual(int line, T1 o1, T2 o2) { if (o1 > o2) _Fail(line, [this,o1,o2] { Print(o1); Print(" > "); Print(o2); }); }
    template<typename T1, typename T2> void _AssertGreaterOrEqual(int line, T1 o1, T2 o2) { if (o1 < o2) _Fail(line, [this,o1,o2] { Print(o1); Print(" < "); Print(o2); }); }
    void _AssertEqualString(int line, const char* s1, const char* s2) { if (strcmp(s1, s2)) _Fail(line, [this,s1,s2] { Print(s1); Print(" != "); Print(s2); }); }
    void _AssertNotEqualString(int line, const char* s1, const char* s2) { if (!strcmp(s1, s2)) _Fail(line, [this,s1,s2] { Print(s1); Print(" == "); Print(s2); }); }
    void _Fail(int line);
    void _Fail(int line, const char* reason);
    void _Fail(int line, const char* format, ...);
    void _Fail(int line, std::function<void(void)> reasonPrint);

    void Print(const char* text) { printf("%s", text); }
    void Print(int value) { printf("%d", value); }
    template<typename T> void Print(const T* ptr) { printf("%p", ptr); }
    template<typename T> void Print(T* ptr) { printf("%p", ptr); }
    // everything else is printed as an integer
    template<typename T> void Print(T value) { printf("%ld", (long)value); }

private:
    bool Execute();

    static TestCase* s_first;
    static TestCase* s_last;
    TestCase* next;
    jmp_buf* failJump;
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
