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

#include <clingcon/config.h>


namespace clingcon
{

//for testing
static Config lazySolveConfigProp1 = Config(1000,false,1000, 10000, 1);
static Config lazySolveConfigProp2 = Config(1000,false,1000, 10000, 2);
static Config lazySolveConfigProp3 = Config(1000,false,1000, 10000, 3);
static Config lazySolveConfigProp4 = Config(1000,false,1000, 10000, 4);
// actually not non lazy, just creates all literals, but no constraints are translated
static Config nonlazySolveConfig = Config(0,false,-1,10000,4);
static Config lazyDiffSolveConfig = Config(0,false,1000,10000,4);
static Config translateConfig = Config(-1,false,1000,10000,4);

static std::vector<Config> conf1({lazySolveConfigProp1,lazySolveConfigProp2,lazySolveConfigProp3,lazySolveConfigProp4,nonlazySolveConfig});
}
