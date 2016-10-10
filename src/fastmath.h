// This file is derived from Geometric Tools, Mathematics Functions
// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2016
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt

#ifndef _FASTMATH_H
#define _FASTMATH_H

#include <cmath>

// |x| <= 1
// max error: 2.5477724974187765e-6
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

// max error: 2.5477724974187765e-6
template <typename T>
T Atan(T x)
{
    if (x * x < 1)
        return Atan1(x);
    return std::copysign(T(M_PI_2), x) - Atan1(1 / x);
}


#endif // _FASTMATH_H
