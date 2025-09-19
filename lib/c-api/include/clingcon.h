// {{{ MIT License
//
// Copyright Roland Kaminski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
// }}}

#ifndef CLINGCON_H
#define CLINGCON_H

#include <clingo/theory.h>

//! Major version number.
#define CLINGCON_VERSION_MAJOR 5
//! Minor version number.
#define CLINGCON_VERSION_MINOR 3
//! Revision number.
#define CLINGCON_VERSION_REVISION 0
//! String representation of version.
#define CLINGCON_VERSION "5.3.0"

#ifdef __cplusplus
extern "C" {
#endif

#if defined _WIN32 || defined __CYGWIN__
#define CLINGCON_WIN
#endif
#ifdef CLINGCON_NO_VISIBILITY
#define CLINGCON_VISIBILITY_DEFAULT
#define CLINGCON_VISIBILITY_PRIVATE
#else
#ifdef CLINGCON_WIN
#ifdef CLINGCON_BUILD_LIBRARY
#define CLINGCON_VISIBILITY_DEFAULT __declspec(dllexport)
#else
#define CLINGCON_VISIBILITY_DEFAULT __declspec(dllimport)
#endif
#define CLINGCON_VISIBILITY_PRIVATE
#else
#if __GNUC__ >= 4
#define CLINGCON_VISIBILITY_DEFAULT __attribute__((visibility("default")))
#define CLINGCON_VISIBILITY_PRIVATE __attribute__((visibility("hidden")))
#else
#define CLINGCON_VISIBILITY_DEFAULT
#define CLINGCON_VISIBILITY_PRIVATE
#endif
#endif
#endif

CLINGCON_VISIBILITY_DEFAULT bool clingcon_create(clingo_lib_t *lib, clingo_theory_t *theory);

#ifdef __cplusplus
}
#endif

#endif
