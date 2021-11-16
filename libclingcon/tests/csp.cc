// {{{ MIT License
//
// Copyright 2020 Roland Kaminski
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

#include "solve.hh"
#include "catch.hpp"

#include <iostream>

using namespace Clingcon;

TEST_CASE("minimizebug", "[solving]") {
    for (int i = 0; i < 10000; ++i) {
        REQUIRE( solve("&maximize { x }. &sum{ x } <= 0 :- a. {a}.", -3, 3) == S({"x=3"}) );
    }
}

#if false

TEST_CASE("optimize", "[solving]") {
    SECTION("minimize") {
        REQUIRE(solve("&minimize { x }.", -3, 3) == S({"x=-3"}));
        REQUIRE(solve("&minimize { x+6 }.", -3, 3) == S({"x=-3"}));
        REQUIRE(solve("&minimize { 2*x }.", -3, 3) == S({"x=-3"}));
        REQUIRE(solve("&minimize { x+y }.", -3, 3) == S({"x=-3 y=-3"}));
        REQUIRE(solve("&minimize { x+y }. &sum{ x + y } >= 2.", -3, 3) == S({"x=-1 y=3", "x=0 y=2", "x=1 y=1", "x=2 y=0", "x=3 y=-1"}));
        REQUIRE(solve("&minimize { x }. &sum{ x } >= 0 :- a. {a}.", -3, 3) == S({"x=-3"}));
        REQUIRE(solve("&minimize { x }. a :- &sum{ x } <= 0.", -3, 3) == S({"a x=-3"}));
    }
    SECTION("maximize") {
        REQUIRE(solve("&maximize { x }.", -3, 3) == S({"x=3"}));
        REQUIRE(solve("&maximize { x+6 }.", -3, 3) == S({"x=3"}));
        REQUIRE(solve("&maximize { 2*x }.", -3, 3) == S({"x=3"}));
        REQUIRE(solve("&maximize { x+y }.", -3, 3) == S({"x=3 y=3"}));
        REQUIRE(solve("&maximize { x+y }. &sum{ x + y } <= 5.", -3, 3) == S({"x=2 y=3", "x=3 y=2"}));
        REQUIRE(solve("&maximize { x }. &sum{ x } <= 0 :- a. {a}.", -3, 3) == S({"x=3"}));
        REQUIRE(solve("&maximize { x }. a :- &sum{ x } >= 0.", -3, 3) == S({"a x=3"}));
    }
}

#endif
