/*
 * Copyright (c) 2020 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * collections/SelfLinkedList.h
 */

#pragma once

#include <base/base.h>

template<typename T> class SelfLinkedList
{
private:
    T* first = NULL;

public:
    constexpr operator bool() const { return !!first; }

    struct Iterator
    {
        constexpr bool operator !=(const Iterator& other) const { return n != other.n; }
        Iterator& operator++() { n = n->next; return *this; }
        T& operator*() { return *n; }

    private:
        T* n;
        constexpr Iterator(T* n) : n(n) {}

        friend class SelfLinkedList;
    };

    struct Manipulator
    {
        Manipulator& begin() { return *this; }
        Manipulator& end() { return *this; }

        constexpr bool operator !=(const Manipulator& other) const { return *p; }
        constexpr operator bool() const { return *p; }
        Manipulator& operator++() { if (*p) { p = &(*p)->next; } return *this; }
        Manipulator& operator*() { return *this; }
        constexpr T& Element() const { return **p; }

        constexpr bool operator==(const T& element) const { return *p == &element; }
        constexpr bool operator==(const T* element) const { return *p == element; }

        T& Insert(T* element) { ASSERT(element); return Insert(*element); }
        T& Insert(T& element)
        {
            element.next = *p;
            *p = &element;
            return element;
        }

        T& Remove()
        {
            T& element = **p;
            *p = element.next;
            return element;
        }

    private:
        T** p;
        constexpr Manipulator(SelfLinkedList* list) : p(&(list->first)) {}

        friend class SelfLinkedList;
    };

    Iterator begin() { return first; }
    Iterator end() { return NULL; }

    Manipulator Manipulate() { return Manipulator(this); }

    T* First() { return first; }

    T& Push(T& element) { return Manipulate().Insert(element); }
    T& Push(T* element) { return Manipulate().Insert(element); }

    T& Append(T* element) { ASSERT(element); return Append(*element); }
    T& Append(T& element)
    {
        auto manip = Manipulate();
        while (manip) ++manip;
        return manip.Insert(element);
    }

    bool Remove(const T& element) { return Remove(&element); }
    bool Remove(const T* element)
    {
        for (auto man: Manipulate())
        {
            if (man.Element() == element)
            {
                man.Remove();
                return true;
            }
        }
        return false;
    }

    bool Contains(const T& element) const { return Contains(&element); }
    bool Contains(const T* element)
    {
        for (auto& e: *this)
        {
            if (&e == element)
                return true;
        }
        return false;
    }

    void Clear()
    {
        auto man = Manipulate();
        while (man)
        {
            man.Remove();
        }
    }
};
