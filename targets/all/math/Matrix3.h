/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * math/Matrix3.h
 *
 * Simple three-dimensional float matrix
 */

#pragma once

#include <base/base.h>

#include "Vector3.h"

struct Matrix3
{
    union
    {
        float raw[9];
        float m[3][3];
    };

    //! Prepend another matrix to this matrix
    Matrix3 Prepend(const Matrix3& other) const
    {
        auto& a = m;
        auto& b = other.m;
        return { .raw = {
            a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0],
            a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1],
            a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2],
            a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0],
            a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1],
            a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2],
            a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0],
            a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1],
            a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2]
        } };
    }

    //! Append another matrix to this matrix
    Matrix3 Append(const Matrix3& other) const { return other.Prepend(*this); }

    //! Transforms the vector by the matrix
    Vector3 Transform(const Vector3& v) const
    {
        return {
            .x = v.x * m[0][0] + v.y * m[0][1] + v.z * m[0][2],
            .y = v.x * m[1][0] + v.y * m[1][1] + v.z * m[1][2],
            .z = v.x * m[2][0] + v.y * m[2][1] + v.z * m[2][2],
        };
    }

    static constexpr Matrix3 Identity()
    {
        return { .raw = {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
        } };
    }

    static Matrix3 RotateX(float theta)
    {
        float s = sinf(theta), c = cosf(theta);
        return { .raw = {
            1, 0, 0,
            0, c, -s,
            0, s,  c,
        } };
    }

    static Matrix3 RotateY(float theta)
    {
        float s = sinf(theta), c = cosf(theta);
        return { .raw = {
             c, 0, s,
             0, 1, 0,
            -s, 0, c,
        } };
    }

    static Matrix3 RotateZ(float theta)
    {
        float s = sinf(theta), c = cosf(theta);
        return { .raw = {
            c, -s, 0,
            s,  c, 0,
            0,  0, 1,
        } };
    }
};
