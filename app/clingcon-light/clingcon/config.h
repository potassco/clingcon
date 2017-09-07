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

#pragma once
#include <clingcon/types.h>

namespace clingcon
{

struct Config
{
public:
    Config() {} //= default;

    Config(int64 translateConstraints, bool alldistinctCard, int64 minLitsPerVar,
           unsigned int domSize, unsigned int propStrength, bool dontcare)
        : translateConstraints(translateConstraints)
        , alldistinctCard(alldistinctCard)
        , minLitsPerVar(minLitsPerVar)
        , domSize(domSize)
        , propStrength(propStrength)
        , dontcare(dontcare)
    {
    }
    int64 translateConstraints; // translate constraint if expected number of clauses is less than
                                // this number (-1 = all)
    bool alldistinctCard;      /// translate alldistinct with cardinality constraints, default false
    int64 minLitsPerVar;       /// precreate at least this number of literals per
                               /// variable (-1 = all)
    int64 domSize;             /// the maximum number of chunks a domain can have when
                               /// multiplied (if avoidable)
    unsigned int propStrength; /// propagation strength for lazy constraints 1..4
    bool dontcare;             /// option for testing strict/vs fwd/back inferences only
};
}
