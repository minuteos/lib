/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * collections/LinkedList.h
 */

#pragma once

#include <base/base.h>

template<typename T> class LinkedList
{
private:
    struct Node
    {
        Node* next;
        T element;
    };

    Node* first = NULL;

public:
    constexpr operator bool() const { return !!first; }

    struct Iterator
    {
        constexpr bool operator !=(const Iterator& other) const { return n != other.n; }
        Iterator& operator++() { n = n->next; return *this; }
        T& operator*() { return n->element; }

    private:
        Node* n;
        constexpr Iterator(Node* n) : n(n) {}

        friend class LinkedList;
    };

    struct Manipulator
    {
        Manipulator& begin() { return *this; }
        Manipulator& end() { return *this; }

        constexpr bool operator !=(const Manipulator& other) const { return *p; }
        constexpr operator bool() const { return *p; }
        Manipulator& operator++() { if (*p) { p = &(*p)->next; } return *this; }
        Manipulator& operator*() { return *this; }
        constexpr T& Element() const { return (*p)->element; }

        constexpr bool operator==(const Node& node) const { return *p == &node; }

        void Insert(const T& element)
        {
            auto node = MemPoolAlloc<Node>();
            node->next = *p;
            node->element = element;
            *p = node;
        }

        void Remove()
        {
            Node* n = *p;
            *p = n->next;
            MemPoolFree(n);
        }

    private:
        Node** p;
        constexpr Manipulator(LinkedList* list) : p(&(list->first)) {}

        friend class LinkedList;
    };

    Iterator begin() { return first; }
    Iterator end() { return NULL; }

    Manipulator Manipulate() { return Manipulator(this); }

    void Push(const T& element) { Manipulate().Insert(element); }

    bool Remove(const T& element)
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

    void Clear()
    {
        auto man = Manipulate();
        while (man)
        {
            man.Remove();
        }
    }
};
