/*
 * Copyright (c) 2022 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * test/hw/GPIO.h
 */

#pragma once

#include <base/base.h>

struct GPIOPin
{
    uint32_t* port;
    uint32_t mask;

    GPIOPin()
        : port(&s_ports[8]), mask(0) {}
    GPIOPin(uint32_t* port, uint32_t mask)
        : port(port), mask(mask) {}

    const char* Name() const;

    //! Configures the GPIOPin as a digital input
    void ConfigureDigitalInput() const { }
    //! Configures the GPIOPin as a digital input with internal pull-down (@p pullUp == @c false) or pull-up (@p pullUp == @c true) enabled
    void ConfigureDigitalInput(bool pullUp) const { }
    //! Configures the GPIOPin as a digital output
    void ConfigureDigitalOutput(bool set = false, bool alt = false) const { }
    //! Configures the GPIOPin as an open drain output
    void ConfigureOpenDrain(bool set = true) const { }
    //! Configures the GPIOPin as an analog input
    void ConfigureAnalog() const { }
    //! Disables the GPIOPin
    void Disable() const { }

    //! Gets the input state of the GPIOPin
    bool Get() const { return *port & mask; }
    //! Sets the output of the GPIOPin to a logical 1
    void Set() const { *port |= mask; }
    //! Reset the output of the GPIOPin to a logical 0
    void Res() const { *port &= ~mask; }
    //! Sets the output of the GPIOPin to the desired @p state
    void Set(bool state) { state ? Set() : Res(); }
    //! Toggles the output of the GPIOPin
    void Toggle() const { *port ^= mask; }
#ifdef Ckernel
    //! Waits for the pin to have the specified state
    async_once(WaitFor, bool state, Timeout timeout = Timeout::Infinite)
    async_once_def()
    {
        async_return(await_mask_timeout(*port, mask, mask * state, timeout));
    }
    async_end
#endif

    //! Gets the input state of the GPIOPin
    operator bool() const { return Get(); }

private:
    static uint32_t s_ports[];
};

inline uint32_t GPIOPin::s_ports[9];

#define Px       GPIOPin()
