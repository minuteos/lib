/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel-app/InitList.h
 */

#pragma once

#include <kernel/kernel.h>

namespace kernel
{

template<typename T> class InitList
{
    static const T* s_list;
    const T* next;

protected:
    InitList() : next(s_list)
    {
        s_list = (const T*)this;
    }


    static const T* First() { return s_list; }
    const T* Next() const { return next; }

public:
    static bool Empty() { return !s_list; }
    static size_t Count()
    {
        size_t n = 0;
        for (auto p = s_list; p; p = p->next)
            n++;
        return n;
    }
};

template<typename T> const T* InitList<T>::s_list;

}
