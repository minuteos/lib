/*
 * Copyright (c) 2022 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * test/bus/SPI.h
 */

#pragma once

#include <kernel/kernel.h>

#include <hw/GPIO.h>

namespace bus
{

class SPI
{
public:
    typedef GPIOPin ChipSelect;

    //! Represents an single SPI transfer step
    struct Descriptor
    {
        void Transmit(Span d) {}
        void TransmitSame(const uint8_t* src, size_t length) {}
        void Receive(Buffer d) {}
        void ReceiveSame(volatile void* dst, size_t length) {}
        void Bidirectional(Buffer d) { Bidirectional(d, d); }
        void Bidirectional(Span transmit, Buffer receive) {}
        size_t Length() const { return 0; }
    };

    //! Gets the maximum number of bytes that can be transferred using a single transfer descriptor
    constexpr static size_t MaximumTransferSize() { return 128; }
    //! Retrieves a ChipSelect handle for the specified GPIO pin for the current USART
    ChipSelect GetChipSelect(GPIOPin pin) { return pin; }

    //! Acquires the bus for the device identified by the specified @ref ChipSelect
    async(Acquire, ChipSelect cs, Timeout timeout = Timeout::Infinite) async_def_return(0);
    //! Releases the bus
    void Release() {}

    //! Performs a single SPI transfer
    async(Transfer, Descriptor& descriptor) async_def_return(0);
    //! Performs a chain of SPI transfers
    async(Transfer, Descriptor* descriptors, size_t count) async_def_return(0);
    //! Performs a chain of SPI transfers
    template<size_t n> ALWAYS_INLINE async(Transfer, Descriptor (&descriptors)[n]) async_def_return(0);
};

}
