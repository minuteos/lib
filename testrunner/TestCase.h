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
    void Assert(bool condition) { if (!condition) Fail(); }
    template<typename T1, typename T2> void AssertEqual(T1 o1, T2 o2) { if (o1 != o2) Fail([this,o1,o2] { Print(o1); Print(" != "); Print(o2); }); }
    template<typename T1, typename T2> void AssertNotEqual(T1 o1, T2 o2) { if (o1 == o2) Fail([this,o1,o2] { Print(o1); Print(" == "); Print(o2); }); }
    template<typename T1, typename T2> void AssertLessThan(T1 o1, T2 o2) { if (o1 >= o2) Fail([this,o1,o2] { Print(o1); Print(" >= "); Print(o2); }); }
    template<typename T1, typename T2> void AssertGreaterThan(T1 o1, T2 o2) { if (o1 <= o2) Fail([this,o1,o2] { Print(o1); Print(" <= "); Print(o2); }); }
    template<typename T1, typename T2> void AssertLessOrEqual(T1 o1, T2 o2) { if (o1 > o2) Fail([this,o1,o2] { Print(o1); Print(" > "); Print(o2); }); }
    template<typename T1, typename T2> void AssertGreaterOrEqual(T1 o1, T2 o2) { if (o1 < o2) Fail([this,o1,o2] { Print(o1); Print(" < "); Print(o2); }); }
    void AssertEqualString(const char* s1, const char* s2) { if (strcmp(s1, s2)) Fail([this,s1,s2] { Print(s1); Print(" != "); Print(s2); }); }
    void AssertNotEqualString(const char* s1, const char* s2) { if (!strcmp(s1, s2)) Fail([this,s1,s2] { Print(s1); Print(" == "); Print(s2); }); }
    void Fail();
    void Fail(const char* reason);
    void Fail(const char* format, ...);
    void Fail(std::function<void(void)> reasonPrint);
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
