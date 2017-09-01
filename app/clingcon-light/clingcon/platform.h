// {{{ MIT License

// Copyright 2017 Max Ostrowski

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

//
// Copyright (c) 2006-2012, Benjamin Kaufmann
//
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/
//
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Clasp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Clasp; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//


#pragma once


#define STRING2(x) #x
#define STRING(x) STRING2(x)

#if defined(_MSC_VER) && _MSC_VER >= 1200
#define CLASP_PRAGMA_TODO(X) __pragma(message(__FILE__ "(" STRING(__LINE__) ") : TODO: " X))
#define FUNC_NAME __FUNCTION__
#include <basetsd.h>
#if _MSC_VER >= 1600
#include <stdint.h>
#endif
typedef UINT8     uint8;
typedef UINT16    uint16;
typedef INT16     int16;
typedef INT32     int32;
typedef UINT32    uint32;
typedef UINT64    uint64;
typedef INT64     int64;
typedef UINT_PTR  uintp;
typedef INT16     int16;
#define PRIu64 "llu"
#define PRId64 "lld"
template <unsigned> struct Uint_t;
template <> struct Uint_t<sizeof(uint8)>  { typedef uint8  type; };
template <> struct Uint_t<sizeof(uint16)> { typedef uint16 type; };
template <> struct Uint_t<sizeof(uint32)> { typedef uint32 type; };
template <> struct Uint_t<sizeof(uint64)> { typedef uint64 type; };
#define BIT_MASK(x,n) ( static_cast<Uint_t<sizeof((x))>::type>(1) << (n) )
#elif defined(__GNUC__) && __GNUC__ >= 3
#define FUNC_NAME __PRETTY_FUNCTION__
#if !defined(__STDC_FORMAT_MACROS)
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
typedef uint8_t   uint8;
typedef uint16_t  uint16;
typedef int16_t   int16;
typedef int32_t   int32;
typedef uint32_t  uint32;
typedef uint64_t  uint64;
typedef int64_t   int64;
typedef uintptr_t uintp;
#define BIT_MASK(x,n) ( static_cast<__typeof((x))>(1)<<(n) )
#define APPLY_PRAGMA(x) _Pragma (#x)
#else
#error unknown compiler or platform. Please add typedefs manually.
#endif
#ifndef UINT32_MAX
#define UINT32_MAX (~uint32(0))
#endif
#ifndef UINT64_MAX
#define UINT64_MAX (~uint64(0))
#endif
#ifndef INT64_MAX
#define INT64_MAX ((int64)(UINT64_MAX >> 1))
#endif
#ifndef UINTP_MAX
#define UINTP_MAX (~uintp(0))
#endif
#ifndef INT16_MAX
#define INT16_MAX (0x7fff)
#endif
#ifndef INT16_MIN
#define	INT16_MIN (-INT16_MAX - 1)
#endif
#ifndef FUNC_NAME
#define FUNC_NAME __FILE__
#endif


namespace MyClasp {

/*!
 * \defgroup misc Miscellaneous and Internal Stuff not specific to clasp.
 */
//@{

template <class T>
inline T bit_mask(unsigned n) { return static_cast<T>(1) << n; }
// returns whether bit n is set in x
template <class T>
inline bool test_bit(T x, unsigned n) { return (x & bit_mask<T>(n)) != 0; }
template <class T>
inline T clear_bit(T x, unsigned n)   { return x & ~bit_mask<T>(n); }
template <class T>
inline T set_bit(T x, unsigned n)     { return x | bit_mask<T>(n); }
template <class T>
inline T toggle_bit(T x, unsigned n)  { return x ^ bit_mask<T>(n); }
template <class T>
inline T& store_clear_bit(T& x, unsigned n)  { return (x &= ~bit_mask<T>(n)); }
template <class T>
inline T& store_set_bit(T& x, unsigned n)  { return (x |= bit_mask<T>(n)); }
template <class T>
inline T& store_toggle_bit(T& x, unsigned n)  { return (x ^= bit_mask<T>(n)); }
template <class T>
inline T right_most_bit(T x) { return x & (-x); }

}





