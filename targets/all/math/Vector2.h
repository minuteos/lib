/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * math/Vector2.h
 *
 * Simple two-dimensional float vector
 */

#pragma once

#include <base/base.h>

struct Vector2
{
    float x, y;

    //! Calculate the magnitude of the vector
    float Magnitude() const { return sqrtf(x * x + y * y); }

    //! Returns the normal vector
    Vector2 Normal() const
    {
        float mul = 1 / Magnitude();
        return { x * mul, y * mul };
    }

    //! Adds another vector to this vector
    Vector2 operator +=(const Vector2& b);
    //! Subtracts another vector from this vector
    Vector2 operator -=(const Vector2& b);
    //! Scales the vector
    Vector2 operator *=(float b);

    //! Calculate the dot product of two vectors
    static float DotProduct(const Vector2& a, const Vector2& b)
    {
        return a.x * b.x + a.y * b.y;
    }
};

//! Calculate the sum of two vectors
inline Vector2 operator +(const Vector2& a, const Vector2& b)
{
    return { a.x + b.x, a.y + b.y };
}
inline Vector2 Vector2::operator +=(const Vector2& b) { return *this = *this + b; }

//! Calculate the difference of two vectors
inline Vector2 operator -(const Vector2& a, const Vector2& b)
{
    return { a.x - b.x, a.y - b.y };
}
inline Vector2 Vector2::operator -=(const Vector2& b) { return *this = *this - b; }

//! Scale the vector by a constant
inline Vector2 operator *(const Vector2& a, float b)
{
    return { a.x * b, a.y * b };
}
inline Vector2 Vector2::operator *=(float b) { return *this = *this * b; }
