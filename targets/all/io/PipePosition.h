/*
 * Copyright (c) 2020 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * io/PipePosition.h
 */

#pragma once

#include <base/base.h>

namespace io
{

class PipePosition
{
public:
    constexpr PipePosition()
        : pos(0) {}

    constexpr ptrdiff_t operator -(PipePosition other) const { return pos - other.pos; }
    constexpr PipePosition operator +(ptrdiff_t offset) const { return pos + offset; }

    constexpr bool operator <(PipePosition other) const { return OVF_LT(pos, other.pos); }
    constexpr bool operator <=(PipePosition other) const { return OVF_LE(pos, other.pos); }
    constexpr bool operator >(PipePosition other) const { return OVF_GT(pos, other.pos); }
    constexpr bool operator >=(PipePosition other) const { return OVF_GE(pos, other.pos); }

    constexpr bool operator ==(PipePosition other) const { return pos == other.pos; }
    constexpr bool operator !=(PipePosition other) const { return pos != other.pos; }

    constexpr PipePosition& operator +=(ptrdiff_t offset) { pos += offset; return *this; }
    constexpr PipePosition& operator -=(ptrdiff_t offset) { pos -= offset; return *this; }

    constexpr size_t LengthUntil(PipePosition other) const { return other.pos <= pos ? 0 : other.pos - pos; }

private:
    constexpr PipePosition(size_t pos)
        : pos(pos) {}

    size_t pos;

    friend class Pipe;
};

}
