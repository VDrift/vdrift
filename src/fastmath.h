// This file is derived from Geometric Tools, Mathematics Functions
// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2016
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt

#ifndef _FASTMATH_H
#define _FASTMATH_H

#include "minmax.h"
#include <cmath>

#define FM_TAN_ERROR 2.2988173242199927e-6
#define FM_ATAN_ERROR 2.5477724974187765e-6
#define FM_COS_ERROR 9.2028470133065365e-6
#define FM_SIN_ERROR 1.0205878936686563e-6

// |x| <= pi/4
template <typename T>
T TanPi4(T x)
{
    T s = x * x;
    T p = T(4.6097377279281204e-02);
    p = T(3.7696344813028304e-02) + p * s;
    p = T(1.3747843432474838e-01) + p * s;
    p = T(3.3299232843941784e-01) + p * s;
    p = 1 + p * s;
    p = p * x;
    return p;
}

// |x| < pi/2
// error increases close to pi/2
// at 0.99 * pi/2 error is 8E-4
template <typename T>
T TanPi2(T x)
{
    // |x| < pi/4
    if (std::abs(x) < T(M_PI_4))
        return TanPi4(x);

    // |x| < pi/2
    T s = x * x;
    T n = x * (T(10395) - s * (T(1260) - s * T(21)));
    T d = T(10395) - s * (T(4725) - s * (T(210) - s));
    return n / d;
}

// |x| <= 1
template <typename T>
T Atan1(T x)
{
    T s = x * x;
    T p = T(-1.2490720064867844e-02);
    p = T(+5.5063351366968050e-02) + p * s;
    p = T(-1.1921576270475498e-01) + p * s;
    p = T(+1.9498657165383548e-01) + p * s;
    p = T(-3.3294527685374087e-01) + p * s;
    p = 1 + p * s;
    p = p * x;
    return p;
}

template <typename T>
T Atan(T x)
{
    // |x| < 1
    if (x * x < 1)
        return Atan1(x);

    // |x| > 1
    return std::copysign(T(M_PI_2), x) - Atan1(1 / x);
}

// |x| <= pi/2
template <typename T>
T CosPi2(T x)
{
    T s = x * x;
    T p = T(-1.2712435011987822e-03);
    p = T(+4.1493920348353308e-02) + p * s;
    p = T(-4.9992746217057404e-01) + p * s;
    p = 1 + p * s;
    return p;
}

// |x| <= 3/2pi
template <typename T>
T Cos3Pi2(T x)
{
    T y = std::abs(x);
    T z = T(M_PI) - y;
    z = Min(z, y);
    z = CosPi2(z);
    z = std::copysign(z, T(M_PI_2) - y);
    return z;
}

// |x| <= pi/2
template <typename T>
T SinPi2(T x)
{
    T s = x * x;
    T p = T(-1.8447486103462252e-04);
    p = T(+8.3109378830028557e-03) + p * s;
    p = T(-1.6665578084732124e-01) + p * s;
    p = 1 + p * s;
    p = p * x;
    return p;
}

// |x| <= 3/2pi
template <typename T>
T Sin3Pi2(T x)
{
    T y = Clamp(x, -T(M_PI) - x, T(M_PI) - x);
    return SinPi2(y);
}

template <typename T>
T Rsqrt(T x)
{
    return 1 / std::sqrt(x);
}

template <typename T>
T CosAtan(T x)
{
    return Rsqrt(x * x + 1);
}

template <typename T>
T SinAtan(T x)
{
    return x * Rsqrt(x * x + 1);
}

template <typename T>
T Cos2Atan(T x)
{
    return (1 - x * x) / (x * x + 1);
}

template <typename T>
T Sin2Atan(T x)
{
    return 2 * x / (x * x + 1);
}

template <typename T>
T Cos2Atan(T y, T x)
{
    return (x * x - y * y) / (x * x + y * y);
}

template <typename T>
T Sin2Atan(T y, T x)
{
    return 2 * x * y / (x * x + y * y);
}

#endif // _FASTMATH_H
