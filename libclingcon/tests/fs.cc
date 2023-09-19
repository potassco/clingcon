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

#include <regex>

using namespace Clingcon;

namespace {

std::string const FSB = R"(
#const bound=16.
)";

std::string const FSO = R"(
&minimize { bound }.
)";

std::string const FSI = R"(
            machine(1).      machine(2).
task(a). duration(a,1,3). duration(a,2,4).
task(b). duration(b,1,1). duration(b,2,6).
task(c). duration(c,1,5). duration(c,2,5).
)";

std::string const FSE = R"(
1 { cycle(T,U) : task(U), U != T } 1 :- task(T).
1 { cycle(T,U) : task(T), U != T } 1 :- task(U).

reach(M) :- M = #min { T : task(T) }.
reach(U) :- reach(T), cycle(T,U).
:- task(T), not reach(T).

1 { start(T) : task(T) } 1.

permutation(T,U) :- cycle(T,U), not start(U).

seq((T,M),(T,M+1),D) :- task(T), duration(T,M,D), machine(M+1).
seq((T1,M),(T2,M),D) :- permutation(T1,T2), duration(T1,M,D).

&sum {  1*T1 + -1*T2 } <= -D :- seq(T1,T2,D).
&sum { -1*(T,M) } <= 0       :- duration(T,M,D).
&sum {  1*(T,M) } <= bound-D :- duration(T,M,D).

#show permutation/2.
)";

std::string const FSD = R"(
1 { cycle(T,U) : task(U), U != T } 1 :- task(T).
1 { cycle(T,U) : task(T), U != T } 1 :- task(U).

reach(M) :- M = #min { T : task(T) }.
reach(U) :- reach(T), cycle(T,U).
:- task(T), not reach(T).

1 { start(T) : task(T) } 1.

permutation(T,U) :- cycle(T,U), not start(U).

seq((T,M),(T,M+1),D) :- task(T), duration(T,M,D), machine(M+1).
seq((T1,M),(T2,M),D) :- permutation(T1,T2), duration(T1,M,D).

&diff { T1-T2 } <= -D :- seq(T1,T2,D).
&diff { 0-(T,M) } <= 0 :- duration(T,M,D).
&sum { (T,M)-0 } <= bound-D :- duration(T,M,D).

#show permutation/2.
)";

S const SOL16{"permutation(a,c) permutation(b,a) (a,1)=1 (a,2)=7 (b,1)=0 (b,2)=1 (c,1)=4 (c,2)=11",
              "permutation(a,c) permutation(b,a) (a,1)=1 (a,2)=7 (b,1)=0 (b,2)=1 (c,1)=5 (c,2)=11",
              "permutation(a,c) permutation(b,a) (a,1)=1 (a,2)=7 (b,1)=0 (b,2)=1 (c,1)=6 (c,2)=11",
              "permutation(a,c) permutation(b,a) (a,1)=2 (a,2)=7 (b,1)=0 (b,2)=1 (c,1)=5 (c,2)=11",
              "permutation(a,c) permutation(b,a) (a,1)=2 (a,2)=7 (b,1)=0 (b,2)=1 (c,1)=6 (c,2)=11",
              "permutation(a,c) permutation(b,a) (a,1)=3 (a,2)=7 (b,1)=0 (b,2)=1 (c,1)=6 (c,2)=11",
              "permutation(b,c) permutation(c,a) (a,1)=6 (a,2)=12 (b,1)=0 (b,2)=1 (c,1)=1 (c,2)=7",
              "permutation(b,c) permutation(c,a) (a,1)=7 (a,2)=12 (b,1)=0 (b,2)=1 (c,1)=1 (c,2)=7",
              "permutation(b,c) permutation(c,a) (a,1)=7 (a,2)=12 (b,1)=0 (b,2)=1 (c,1)=2 (c,2)=7",
              "permutation(b,c) permutation(c,a) (a,1)=8 (a,2)=12 (b,1)=0 (b,2)=1 (c,1)=1 (c,2)=7",
              "permutation(b,c) permutation(c,a) (a,1)=8 (a,2)=12 (b,1)=0 (b,2)=1 (c,1)=2 (c,2)=7",
              "permutation(b,c) permutation(c,a) (a,1)=9 (a,2)=12 (b,1)=0 (b,2)=1 (c,1)=1 (c,2)=7",
              "permutation(b,c) permutation(c,a) (a,1)=9 (a,2)=12 (b,1)=0 (b,2)=1 (c,1)=2 (c,2)=7"};

S const SOL11{SOL16.begin(), SOL16.begin() + 6};

auto remove_bound(std::string const &str) -> std::string {
    return std::regex_replace(str, std::regex{"bound=[^ ]* "}, "");
}
auto remove_bound(S &&res) -> S {
    for (auto &str : res) {
        str = remove_bound(str);
    }
    return std::move(res);
}

} // namespace

TEST_CASE("fs", "[fs]") {
    SECTION("fse") {
        REQUIRE(solve(FSB + FSE + FSI, 0, 10) == S({}));
        REQUIRE(solve(FSB + FSE + FSI, 0, 11) == SOL11);
        REQUIRE(solve(FSB + FSE + FSI) == SOL16);
    }
    SECTION("fsd") {
        REQUIRE(solve(FSB + FSD + FSI, 0, 10) == S({}));
        REQUIRE(solve(FSB + FSD + FSI, 0, 11) == SOL11);
        REQUIRE(solve(FSB + FSD + FSI) == SOL16);
    }
    SECTION("fso") { REQUIRE(remove_bound(solve(FSO + FSE + FSI, -256, 256)) == SOL16); }
}
