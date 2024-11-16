/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * math/Vector3.h
 *
 * Simple three-dimensional float vector
 */

#pragma once

#include <base/base.h>

struct Vector3
{
    float x, y, z;

    //! Calculate the magnitude of the vector
    float Magnitude() const { return sqrtf(x * x + y * y + z * z); }

    //! Returns the normal vector
    Vector3 Normal() const
    {
        float mul = 1 / Magnitude();
        return { x * mul, y * mul, z * mul };
    }

    //! Adds another vector to this vector
    Vector3 operator +=(const Vector3& b);
    //! Subtracts another vector from this vector
    Vector3 operator -=(const Vector3& b);
    //! Scales the vector
    Vector3 operator *=(float b);

    //! Calculate the dot product of two vectors
    static float DotProduct(const Vector3& a, const Vector3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    //! Calculate the cross product of two vectors
    static Vector3 CrossProduct(const Vector3& a, const Vector3& b)
    {
        return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
    }
};

//! Calculate the sum of two vectors
inline Vector3 operator +(const Vector3& a, const Vector3& b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}
inline Vector3 Vector3::operator +=(const Vector3& b) { return *this = *this + b; }

//! Calculate the difference of two vectors
inline Vector3 operator -(const Vector3& a, const Vector3& b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}
inline Vector3 Vector3::operator -=(const Vector3& b) { return *this = *this - b; }

//! Scale the vector by a constant
inline Vector3 operator *(const Vector3& a, float b)
{
    return { a.x * b, a.y * b, a.z * b };
}
inline Vector3 Vector3::operator *=(float b) { return *this = *this * b; }
