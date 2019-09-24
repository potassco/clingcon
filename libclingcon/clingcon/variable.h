// {{{ MIT License

// Copyright 2018 Max Ostrowski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#pragma once
#include "clingcon/types.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <tuple>
//#include <cstddef>

namespace clingcon
{
/// change to 32bit to safe some space
using Variable = unsigned int;
// const Variable InvalidVar = std::numeric_limits<unsigned int>::max();
const Variable InvalidVar = 4294967295;

/// represents a View on a Variable  which is equal to  (a*v) + c
struct View
{
    View(Variable v = 0, int32 a = 1, int32 c = 0)
        : v(v)
        , a(a)
        , c(c)
    {
    }
    Variable v;
    int32 a;
    int32 c;

    /// a*x + c = rhs
    /// returns x given rhs
    int64 divide(int64 rhs) const
    {
        return static_cast< int64 >(reversed() ? std::ceil(static_cast< double >(rhs - c) / a) :
                                                 std::floor(static_cast< double >(rhs - c) / a));
    }

    /// return rhs given a
    int64 multiply(int64 x) const
    {
        return static_cast< int64 >(a) * static_cast< int64 >(x) + static_cast< int64 >(c);
    }

    bool reversed() const { return a < 0; }

    inline View &operator*=(int32 x)
    {
        a *= x;
        c *= x;
        return *this;
    }
    inline View &operator+=(int32 x)
    {
        c += x;
        return *this;
    }
};

inline bool operator<(const View &v1, const View &v2)
{
    return std::tie(v1.v, v1.a, v1.c) < std::tie(v2.v, v2.a, v2.c);
}
inline bool operator==(const View &v1, const View &v2)
{
    return std::tie(v1.v, v1.a, v1.c) == std::tie(v2.v, v2.a, v2.c);
}
inline bool operator!=(const View &v1, const View &v2)
{
    return std::tie(v1.v, v1.a, v1.c) != std::tie(v2.v, v2.a, v2.c);
}
inline View operator*(const View &v1, int32 x) { return View(v1.v, v1.a * x, v1.c * x); }
inline View operator+(const View &v1, int32 x) { return View(v1.v, v1.a, v1.c + x); }
} // namespace clingcon
