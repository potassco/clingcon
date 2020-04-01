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

TEST_CASE("solving", "[solving]") {
    SECTION("simple") {
        REQUIRE(solve("&sum{ x } > 0. &sum{ x } < 3. &sum { x } = y.") == S({
            "x=1 y=1",
            "x=2 y=2"}));
        REQUIRE(solve(
            "&sum {   1 *y + (-5)*x } <= 0. "
            "&sum { (-1)*y +   5 *x } <= 0. "
            "&sum { 15*x } <= 15. "
            "&sum { 10*x } <= 7.", -20, 20) == S({
                "x=-1 y=-5",
                "x=-2 y=-10",
                "x=-3 y=-15",
                "x=-4 y=-20",
                "x=0 y=0"}));
        REQUIRE(solve(
            "&show { even; odd }. "
            "&sum {   1 *even + (-2)*i } <= 0. "
            "&sum { (-1)*even +   2 *i } <= 0. "
            "&sum {   1 *odd + (-2)*i } <=  1. "
            "&sum { (-1)*odd +   2 *i } <= -1.", -2, 2) == S({
                "even=-2 odd=-1",
                "even=0 odd=1"}));
        REQUIRE(solve(
            "a :- &sum{-1*x} <= 0. "
            "b :- &sum{1*x} <= 5. "
            ":- not a. "
            ":- not b.") == S({
                "a b x=0",
                "a b x=1",
                "a b x=2",
                "a b x=3",
                "a b x=4",
                "a b x=5"}));
        REQUIRE(solve(
            "&sum { 1 * x + (-1) * y } <= -1. "
            "&sum { 1 * y + (-1) * x } <= -1.", -20, 20) == S({ }));
        REQUIRE(solve(
            "&sum { 1 } <= 2. ") == S({ "" }));
        REQUIRE(solve(
            "&sum { 2 } <= 1. ").empty());
        REQUIRE(solve(
            "{a}. "
            "&sum {   1 *x } <= -5 :- a. "
            "&sum { (-1)*x } <= -5 :- not a.", -6, 6) == S({
                "a x=-5",
                "a x=-6",
                "x=5",
                "x=6"}));
        REQUIRE(solve("{a}. &sum { x } >= 3 :- a. &sum { x } <= 0 :- not a.", 0, 3) == S({"a x=3", "x=0"}));
    }
    SECTION("parse") {
        REQUIRE(solve("&sum { x(f(1+2)) } <= 0.", 0, 0) == S({"x(f(3))=0"}));
        REQUIRE(solve("&sum { x(f(1-2)) } <= 0.", 0, 0) == S({"x(f(-1))=0"}));
        REQUIRE(solve("&sum { x(f(-2)) } <= 0.", 0, 0) == S({"x(f(-2))=0"}));
        REQUIRE(solve("&sum { x(f(2*2)) } <= 0.", 0, 0) == S({"x(f(4))=0"}));
        REQUIRE(solve("&sum { x(f(4/2)) } <= 0.", 0, 0) == S({"x(f(2))=0"}));
        REQUIRE(solve("&sum { x(f(9\\2)) } <= 0.", 0, 0) == S({"x(f(1))=0"}));
        REQUIRE(solve("&sum { (a,b) } <= 0.", 0, 0) == S({"(a,b)=0"}));
        REQUIRE(solve("&sum { x } != 5.", 0, 0) == S({"x=0"}));
        REQUIRE(solve("&sum { x } = 5.") == S({"x=5"}));
        REQUIRE(solve("&sum { x } != 0.", -1, 1) == S({"x=-1", "x=1"}));
        REQUIRE(solve("&sum { x } < 2.", 0, 3) == S({"x=0", "x=1"}));
        REQUIRE(solve("&sum { x } <= 2.", 0, 3) == S({"x=0", "x=1", "x=2"}));
        REQUIRE(solve("&sum { x } > 1.", 0, 3) == S({"x=2", "x=3"}));
        REQUIRE(solve("&sum { x } >= 1.", 0, 3) == S({"x=1", "x=2", "x=3"}));
        REQUIRE(solve("a :- &sum { x } >= 1.", -3, 3) == S(
            {"a x=1", "a x=2", "a x=3", "x=-1", "x=-2", "x=-3", "x=0"}));
        REQUIRE(solve("a :- &sum { x } = 1.", -3, 3) == S(
            {"a x=1", "x=-1", "x=-2", "x=-3", "x=0", "x=2", "x=3"}));
        REQUIRE(solve("&sum { 5*x + 10*y } = 20.", -3, 3) == S(
            {"x=-2 y=3", "x=0 y=2", "x=2 y=1"}));
        REQUIRE(solve("&sum { -5*x + 10*y } = 20.", -3, 3) == S(
            {"x=-2 y=1", "x=0 y=2", "x=2 y=3"}));
    }
    SECTION("singleton") {
        REQUIRE(solve("&sum { x } <= 1.", 0, 2) == S({"x=0", "x=1"}));
        REQUIRE(solve("&sum { x } >= 1.", 0, 2) == S({"x=1", "x=2"}));
        REQUIRE(solve("a :- &sum { x } <= 1.", 0, 2) == S({"a x=0", "a x=1", "x=2"}));
        REQUIRE(solve(":- &sum { x } <= 1.", 0, 2) == S({"x=2"}));
        REQUIRE(solve(":- not &sum { x } <= 1.", 0, 2) == S({"x=0", "x=1"}));
        REQUIRE(solve("a :- &sum { x } <= 1. b :- not &sum { x } > 1.", 0, 2) == S({"a b x=0", "a b x=1", "x=2"}));
        REQUIRE(solve(" :- &sum { x } <= 1. :- not &sum { x } > 1.", 0, 2) == S({"x=2"}));
    }
    SECTION("dom") {
        REQUIRE(solve("&dom { 0;1..2;2..3;5 } = x.") == S({"x=0", "x=1", "x=2", "x=3", "x=5"}));
        REQUIRE(solve("1 {a; b} 1. &dom { 0;2;4 } = x :- a. &dom { 1;3;5 } = x :- b.") == S({
            "a x=0", "a x=2", "a x=4",
            "b x=1", "b x=3", "b x=5"}));
        REQUIRE(solve("&dom { 0-1..0+1 } = x.") == S({"x=-1", "x=0", "x=1"}));
    }
    SECTION("shift") {
        REQUIRE(solve("{a}. :- a, &sum { x } < 3. :- not a, &sum { x } > 0.", 0, 3) == S({"a x=3", "x=0"}));
    }
    SECTION("show") {
        REQUIRE(solve("&sum { p(X) } = 0 :- X=1..3. &sum { q(X) } = 0 :- X=1..3. &show { }.") == S({
            ""}));
        REQUIRE(solve("&sum { p(X) } = 0 :- X=1..3. &sum { q(X) } = 0 :- X=1..3. &show { p/1 }.") == S({
            "p(1)=0 p(2)=0 p(3)=0"}));
        REQUIRE(solve("&sum { p(X) } = 0 :- X=1..3. &sum { q(X) } = 0 :- X=1..3. &show { p(1); p(2); q(1) }.") == S({
            "p(1)=0 p(2)=0 q(1)=0"}));
        REQUIRE(solve("&sum { p(X) } = 0 :- X=1..3. &sum { q(X) } = 0 :- X=1..3. &show { p/1; q(1) }.") == S({
            "p(1)=0 p(2)=0 p(3)=0 q(1)=0"}));
    }
    SECTION("set") {
        REQUIRE(solve("&sum { 1;1 } = x.") == S({"x=2"}));
        REQUIRE(solve("&sum { 1 : X=1..3 } = x.") == S({"x=3"}));
        REQUIRE(solve("&sum { 1 : X=(1;2;3) } = x.") == S({"x=3"}));
        REQUIRE(solve("&sum { 1 : 1=(X;Y;Z) } = x.") == S({"x=3"}));
        REQUIRE(solve("&sum { v(X) } = X :- X=1..3. &sum { v(X) : X=1..2; v(X) : X=2..3 } = x.") == S({
            "x=8 v(1)=1 v(2)=2 v(3)=3"}));
    }
    SECTION("simple distinct") {
        REQUIRE(solve("&distinct { x; y }.", 0, 1) == S({"x=0 y=1", "x=1 y=0"}));
        REQUIRE(solve("&distinct { 2*x; 3*y }.", 2, 3) == S({"x=2 y=2", "x=2 y=3", "x=3 y=3"}));
        REQUIRE(solve("&distinct { 0*x; 0*y }.", 0, 1) == S({}));
        REQUIRE(solve("&distinct { x; -x }.", 0, 1) == S({"x=1"}));
        REQUIRE(solve("&distinct { x; x+1 }.", 0, 1) == S({"x=0", "x=1"}));
        REQUIRE(solve("&distinct { 0 }.", 0, 1) == S({""}));
        REQUIRE(solve("&distinct { 0; 0 }.", 0, 1) == S({}));
        REQUIRE(solve("&distinct { 0; 0+0 }.", 0, 1) == S({}));
        REQUIRE(solve("&distinct { 0; 1 }.", 0, 1) == S({""}));
        REQUIRE(solve("&distinct { 2*x; (1+1)*x }.", 0, 1) == S({}));
        REQUIRE(solve("&distinct { y-x; x-y }.", 0, 1) == S({"x=0 y=1", "x=1 y=0"}));
        REQUIRE(solve("&distinct { x; y } :- c. &sum { x } = y :- not c. {c}.", 0, 1) == S({
            "c x=0 y=1",
            "c x=1 y=0",
            "x=0 y=0",
            "x=1 y=1"}));
    }
    /*
    def test_optimize_bound(self):
        sol = [[('x', 0), ('y', 2), ('z', 0)],
               [('x', 1), ('y', 1), ('z', 1)],
               [('x', 2), ('y', 0), ('z', 2)]]
        for translate_minimize in (True, False):
            s = Solver(0, 3)
            s.prp.config.translate_minimize.value = translate_minimize
            s.solve("&minimize { x + 2 * y + z + 5 }. &sum{ x + y } >= 2. &sum { y + z } >= 2.")
            self.assertEqual(s.bound, 9)
            self.assertEqual(s.solve("", optimize=False, bound=9), sol)
            self.assertEqual(s.solve("", optimize=False, bound=9), sol)
            self.assertEqual(s.solve("&minimize { 6 }.", optimize=False, bound=9), [])
            self.assertEqual(s.solve("", optimize=False, bound=15), sol)

    def test_optimize(self):
        self.assertEqual(solve("&minimize { x }.", -3, 3), [[('x', -3)]])
        self.assertEqual(solve("&minimize { x+6 }.", -3, 3), [[('x', -3)]])
        self.assertEqual(solve("&maximize { 2*x }.", -3, 3), [[('x', 3)]])
        self.assertEqual(solve("&maximize { x + y }. ", -3, 3), [[('x', 3), ('y', 3)]])
        self.assertEqual(solve("&maximize { x + y }. &sum{ x + y} <= 5. ", -3, 3), [[('x', 2), ('y', 3)], [('x', 3), ('y', 2)]])
        self.assertEqual(solve("&maximize { x }. &sum{ x } <= 0 :- a. {a}. ", -3, 3), [[('x', 3)]])
        self.assertEqual(solve("&minimize { x }. &sum{ x } <= 0 :- a. {a}. ", -3, 3), [[('x', -3)], [('a'), ('x', -3)]])
        self.assertEqual(solve("&minimize { x }. a :- &sum{ x } <= 0. ", -3, 3), [[('a'), ('x', -3)]])

    def test_multishot(self):
        s = Solver(0, 3)
        self.assertEqual(s.solve("&sum { x } <= 2."), [[('x', 0)], [('x', 1)], [('x', 2)]])
        self.assertEqual(s.solve(""), [[('x', 0)], [('x', 1)], [('x', 2)]])
        self.assertEqual(s.solve("&sum { x } <= 1."), [[('x', 0)], [('x', 1)]])
        self.assertEqual(s.solve("&sum { x } <= 0."), [[('x', 0)]])
        self.assertEqual(s.solve("&sum { x } <= 1."), [[('x', 0)]])
        self.assertEqual(s.solve("&sum { x } <= 2."), [[('x', 0)]])

    def test_distinct(self):
        self.assertEqual(solve("&dom{1..1}=x. &dom{1..2}=y. &dom{1..3}=z. &distinct{x;y;z}."), [[('x', 1), ('y', 2), ('z', 3)]])
        self.assertEqual(solve("&dom{1..3}=x. &dom{2..3}=y. &dom{3..3}=z. &distinct{x;y;z}."), [[('x', 1), ('y', 2), ('z', 3)]])
        self.assertEqual(
            solve("""\
            &dom { 1..2 } = x.
            &dom { 1..2 } = y.
            &dom { 1..2 } = z.
            &distinct { 2*x+3*y+5*z; 5*x+2*y+3*z; 3*x+5*y+2*z }.
            """),
            [[('x', 1), ('y', 1), ('z', 2)],
             [('x', 1), ('y', 2), ('z', 1)],
             [('x', 1), ('y', 2), ('z', 2)],
             [('x', 2), ('y', 1), ('z', 1)],
             [('x', 2), ('y', 1), ('z', 2)],
             [('x', 2), ('y', 2), ('z', 1)]])
        self.assertEqual(
            solve("""\
            &dom { 1..2 } = x.
            &dom { 1..2 } = y.
            &dom { 1..2 } = z.
            &distinct { 2*x+3*y+5*z+1; 5*x+2*y+3*z; 3*x+5*y+2*z-1 }.
            """),
            [[('x', 1), ('y', 2), ('z', 2)],
             [('x', 2), ('y', 1), ('z', 1)],
             [('x', 2), ('y', 2), ('z', 1)]])

    */
}
