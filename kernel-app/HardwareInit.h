/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * kernel-app/HardwareInit.h
 */

#pragma once

#include <kernel/kernel.h>

#include <kernel-app/InitList.h>

namespace kernel
{

class HardwareInit : public InitList<HardwareInit>
{
    void (*init)();

public:
    HardwareInit(void (*init)())
        : init(init) {}

    static void Execute()
    {
        for (auto i = InitList<HardwareInit>::First(); i; i = i->Next())
            i->init();
    }
};

}

#define HARDWARE_INIT(...) static kernel::HardwareInit UNIQUE(__hardwareInit)(__VA_ARGS__)
