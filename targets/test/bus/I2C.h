/*
 * Copyright (c) 2022 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * test/bus/I2C.h
 */

#pragma once

#include <kernel/kernel.h>

namespace bus
{

class I2C
{
public:
    //! Starts or restarts a read transaction, reading the specified number of bytes from the bus
    async(Read, uint8_t address, Buffer data, bool start, bool stop) async_def_return(0);
    //! Continues a read transaction, reading the specified number of bytes from the bus
    async(Read, Buffer data, bool stop) async_def_return(0);

    //! Starts or restarts a write transaction, writing the specified number of bytes to the bus
    async(Write, uint8_t address, Span data, bool start, bool stop) async_def_return(0);
    //! Continues a write transaction, writing the specified number of bytes to the bus
    async(Write, Span data, bool stop) async_def_return(0);

    //! Gets the current bus frequency
    uint32_t OutputFrequency() const { return 100000; }
    //! Sets the current bus frequency
    void OutputFrequency(uint32_t frequency) const { }
};

}
