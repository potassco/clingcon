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
#include <catch2/catch_test_macros.hpp>

using namespace Clingcon;

namespace {

std::string const DOM = R"(
&sum { X } <= 9 :- letter(X).
&sum { X } >= 0 :- letter(X).
)";

std::string const DOMC = R"(
&dom {0..9} = X :- letter(X).
)";

std::string const DIST = R"(
&sum { X } != Y :- letter(X), letter(Y), X < Y.
)";

std::string const DISTC = R"(
&distinct {X : letter(X)}.
)";

std::string const SMM = R"(
letter(s;e;n;d;m;o;r;y).

&sum {             1000*s + 100*e + 10*n + 1*d
     ;             1000*m + 100*o + 10*r + 1*e
     } = 10000*m + 1000*o + 100*n + 10*e + 1*y.
&sum { m } != 0.

#show.
&show {X : letter(X)}.
)";

} // namespace

TEST_CASE("money", "[money]") {
    SECTION("money") {
        REQUIRE(solve(DIST + DOM + SMM) == S({"d=7 e=5 m=1 n=6 o=0 r=8 s=9 y=2"}));
        REQUIRE(solve(DIST + DOMC + SMM) == S({"d=7 e=5 m=1 n=6 o=0 r=8 s=9 y=2"}));
        REQUIRE(solve(DIST + SMM, 0, 9) == S({"d=7 e=5 m=1 n=6 o=0 r=8 s=9 y=2"}));
        REQUIRE(solve(DIST + SMM, 0, 10) == S({"d=5 e=8 m=1 n=0 o=2 r=7 s=10 y=3", "d=6 e=8 m=1 n=0 o=2 r=7 s=10 y=4",
                                               "d=7 e=5 m=1 n=6 o=0 r=8 s=9 y=2"}));

        REQUIRE(solve(DISTC + DOM + SMM) == S({"d=7 e=5 m=1 n=6 o=0 r=8 s=9 y=2"}));
        REQUIRE(solve(DISTC + DOMC + SMM) == S({"d=7 e=5 m=1 n=6 o=0 r=8 s=9 y=2"}));
        REQUIRE(solve(DISTC + SMM, 0, 9) == S({"d=7 e=5 m=1 n=6 o=0 r=8 s=9 y=2"}));
        REQUIRE(solve(DISTC + SMM, 0, 10) == S({"d=5 e=8 m=1 n=0 o=2 r=7 s=10 y=3", "d=6 e=8 m=1 n=0 o=2 r=7 s=10 y=4",
                                                "d=7 e=5 m=1 n=6 o=0 r=8 s=9 y=2"}));
    }
}
