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

#include <iostream>

using namespace Clingcon;

TEST_CASE("disjoint", "[solving]") {
    REQUIRE(solve("&dom{1..2}=x. &dom{1..1}=y. {a}. &disjoint{x@1;y@1} :- a.") ==
            S({"a x=2 y=1", "x=1 y=1", "x=2 y=1"}));
    REQUIRE(solve("&dom{1..2}=x. &dom{1..1}=y. &dom{1..1}=z. {a}. &disjoint{x@1;y@1;z@1} :- a.") ==
            S({"x=1 y=1 z=1", "x=2 y=1 z=1"}));
    REQUIRE(solve("&dom{1..2}=x. &dom{1..1}=y. &dom{3..3}=z. {a}. &disjoint{x@1;y@1;z@1} :- a.") ==
            S({"a x=2 y=1 z=3", "x=1 y=1 z=3", "x=2 y=1 z=3"}));
    REQUIRE(solve("&dom{1..4}=x. &dom{1..4}=y. &disjoint{x@3;y@3}.") == S({"x=1 y=4", "x=4 y=1"}));
    REQUIRE(solve("&dom{1..5}=x. &dom{1..5}=y. &dom{1..5}=z. &disjoint{x@2; y@2; z@2}.") ==
            S({"x=1 y=3 z=5", "x=1 y=5 z=3", "x=3 y=1 z=5", "x=3 y=5 z=1", "x=5 y=1 z=3", "x=5 y=3 z=1"}));
    REQUIRE(solve("#const n = 6. "
                  "#show. "
                  "&show { q/1 }. "
                  "p(1..n). "
                  "&dom { 1..n } = q(N) :- p(N). "
                  "&sum { r(N) } = q(N)-N :- p(N). "
                  "&sum { s(N) } = q(N)+N :- p(N). "
                  "&disjoint { q(N)@1 : p(N) }. "
                  "&disjoint { r(N)@1 : p(N) }. "
                  "&disjoint { s(N)@1 : p(N) }. ") ==
            S({"q(1)=2 q(2)=4 q(3)=6 q(4)=1 q(5)=3 q(6)=5", "q(1)=3 q(2)=6 q(3)=2 q(4)=5 q(5)=1 q(6)=4",
               "q(1)=4 q(2)=1 q(3)=5 q(4)=2 q(5)=6 q(6)=3", "q(1)=5 q(2)=3 q(3)=1 q(4)=6 q(5)=4 q(6)=2"}));
}

TEST_CASE("distinct", "[solving]") {
    SECTION("simple") {
        REQUIRE(solve("&distinct { 1; 3; x }.", 2, 3) == S({"x=2"}));
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
        REQUIRE(solve("&distinct { x; y } :- c. &sum { x } = y :- not c. {c}.", 0, 1) ==
                S({"c x=0 y=1", "c x=1 y=0", "x=0 y=0", "x=1 y=1"}));
    }
    SECTION("complex") {
        REQUIRE(solve("&dom{1..2}=x. &dom{1..1}=y. {a}. &distinct{x;y} :- a.") ==
                S({"a x=2 y=1", "x=1 y=1", "x=2 y=1"}));
        REQUIRE(solve("&dom{1..2}=x. &dom{1..1}=y. &dom{1..1}=z. {a}. &distinct{x;y;z} :- a.") ==
                S({"x=1 y=1 z=1", "x=2 y=1 z=1"}));
        REQUIRE(solve("&dom{1..2}=x. &dom{1..1}=y. &dom{3..3}=z. {a}. &distinct{x;y;z} :- a.") ==
                S({"a x=2 y=1 z=3", "x=1 y=1 z=3", "x=2 y=1 z=3"}));
        REQUIRE(solve("&dom{1..1}=x. &dom{1..2}=y. &dom{1..3}=z. &distinct{x;y;z}.") == S({"x=1 y=2 z=3"}));
        REQUIRE(solve("&dom{1..3}=x. &dom{2..3}=y. &dom{3..3}=z. &distinct{x;y;z}.") == S({"x=1 y=2 z=3"}));
        REQUIRE(solve("&dom { 1..2 } = x. &dom { 1..2 } = y. &dom { 1..2 } = z. "
                      "&distinct { 2*x+3*y+5*z; 5*x+2*y+3*z; 3*x+5*y+2*z }.") ==
                solve("&dom { 1..2 } = x. &dom { 1..2 } = y. &dom { 1..2 } = z. "
                      "&sum { 2*x+3*y+5*z } != 5*x+2*y+3*z. "
                      "&sum { 2*x+3*y+5*z } != 3*x+5*y+2*z. "
                      "&sum { 5*x+2*y+3*z } != 3*x+5*y+2*z. "));
        REQUIRE(solve("&dom { 1..2 } = x. &dom { 1..2 } = y. &dom { 1..2 } = z. "
                      "&distinct { 2*x+3*y+5*z+1; 5*x+2*y+3*z; 3*x+5*y+2*z-1 }.") ==
                solve("&dom { 1..2 } = x. &dom { 1..2 } = y. &dom { 1..2 } = z. "
                      "&sum { 2*x+3*y+5*z+1 } != 5*x+2*y+3*z+0. "
                      "&sum { 2*x+3*y+5*z+1 } != 3*x+5*y+2*z-1. "
                      "&sum { 5*x+2*y+3*z+0 } != 3*x+5*y+2*z-1. "));
        REQUIRE(solve("&dom{1..2}=x. &dom{1..3}=y. &dom{1..4}=z. &distinct{1;x;y;z}.") == S({"x=2 y=3 z=4"}));
        REQUIRE(solve("&dom{1..2}=x. &dom{1..4}=y. &dom{1..5}=z. &distinct{1;3;x;y;z}.") == S({"x=2 y=4 z=5"}));
        REQUIRE(solve("#const l = 17. "
                      "#const o = 6. "
                      "&dom { 1..l } = p(P) :- P=1..o. "
                      "&sum { p(1) } = 1. "
                      "&sum { p(P) } < p(P+1) :- P=1..o-1. "
                      "&distinct{ p(Q) - p(P) : P < Q, P=1..o-1, Q=P+1..o }. ")
                    .empty());
        // Note: We get twice a much as on wikipedia because we get them the
        // other way round too.
        REQUIRE(solve("#const l = 18. "
                      "#const o = 6. "
                      "&dom { 1..l } = p(P) :- P=1..o. "
                      "&sum { p(1) } = 1. "
                      "&sum { p(P) } < p(P+1) :- P=1..o-1. "
                      "&distinct{ p(Q) - p(P) : P < Q, P=1..o-1, Q=P+1..o }. ") ==
                S({"p(1)=1 p(2)=2 p(3)=5 p(4)=11 p(5)=13 p(6)=18", "p(1)=1 p(2)=2 p(3)=5 p(4)=11 p(5)=16 p(6)=18",
                   "p(1)=1 p(2)=2 p(3)=9 p(4)=12 p(5)=14 p(6)=18", "p(1)=1 p(2)=2 p(3)=9 p(4)=13 p(5)=15 p(6)=18",
                   "p(1)=1 p(2)=3 p(3)=8 p(4)=14 p(5)=17 p(6)=18", "p(1)=1 p(2)=4 p(3)=6 p(4)=10 p(5)=17 p(6)=18",
                   "p(1)=1 p(2)=5 p(3)=7 p(4)=10 p(5)=17 p(6)=18", "p(1)=1 p(2)=6 p(3)=8 p(4)=14 p(5)=17 p(6)=18"}));
        REQUIRE(solve("#const n = 6. "
                      "#show."
                      "p(1..n). "
                      "&dom { 1..n } = q(N) :- p(N). "
                      "&distinct { q(N)+0 : p(N) }. "
                      "&distinct { q(N)-N : p(N) }. "
                      "&distinct { q(N)+N : p(N) }. ") ==
                S({"q(1)=2 q(2)=4 q(3)=6 q(4)=1 q(5)=3 q(6)=5", "q(1)=3 q(2)=6 q(3)=2 q(4)=5 q(5)=1 q(6)=4",
                   "q(1)=4 q(2)=1 q(3)=5 q(4)=2 q(5)=6 q(6)=3", "q(1)=5 q(2)=3 q(3)=1 q(4)=6 q(5)=4 q(6)=2"}));
        REQUIRE(solve("&distinct { 2*x+7; 3*y+2; 5*z+3}.", 2, 3) ==
                S({{"x=2 y=2 z=2", "x=2 y=2 z=3", "x=3 y=2 z=3", "x=3 y=3 z=3"}}));
    }
}

TEST_CASE("optimize", "[solving]") {
    SECTION("minimize") {
        REQUIRE(solve("&minimize { x }.", -3, 3) == S({"x=-3"}));
        REQUIRE(solve("&minimize { x+6 }.", -3, 3) == S({"x=-3"}));
        REQUIRE(solve("&minimize { 2*x }.", -3, 3) == S({"x=-3"}));
        REQUIRE(solve("&minimize { x+y }.", -3, 3) == S({"x=-3 y=-3"}));
        REQUIRE(solve("&minimize { x+y }. &sum{ x + y } >= 2.", -3, 3) ==
                S({"x=-1 y=3", "x=0 y=2", "x=1 y=1", "x=2 y=0", "x=3 y=-1"}));
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

TEST_CASE("dom", "[solving]") {
    SECTION("dom") {
        REQUIRE(solve("&dom { 0;1..2;2..3;5 } = x.") == S({"x=0", "x=1", "x=2", "x=3", "x=5"}));
        REQUIRE(solve("1 {a; b} 1. &dom { 0;2;4 } = x :- a. &dom { 1;3;5 } = x :- b.") ==
                S({"a x=0", "a x=2", "a x=4", "b x=1", "b x=3", "b x=5"}));
        REQUIRE(solve("&dom { 0-1..0+1 } = x.") == S({"x=-1", "x=0", "x=1"}));
    }
}

TEST_CASE("sum", "[solving]") {
    SECTION("simple") {
        REQUIRE(solve("&sum{ x } > 0. &sum{ x } < 3. &sum { x } = y.") == S({"x=1 y=1", "x=2 y=2"}));
        REQUIRE(solve("&sum {   1 *y + (-5)*x } <= 0. "
                      "&sum { (-1)*y +   5 *x } <= 0. "
                      "&sum { 15*x } <= 15. "
                      "&sum { 10*x } <= 7.",
                      -20, 20) == S({"x=-1 y=-5", "x=-2 y=-10", "x=-3 y=-15", "x=-4 y=-20", "x=0 y=0"}));
        REQUIRE(solve("&show { even; odd }. "
                      "&sum {   1 *even + (-2)*i } <= 0. "
                      "&sum { (-1)*even +   2 *i } <= 0. "
                      "&sum {   1 *odd + (-2)*i } <=  1. "
                      "&sum { (-1)*odd +   2 *i } <= -1.",
                      -2, 2) == S({"even=-2 odd=-1", "even=0 odd=1"}));
        REQUIRE(solve("a :- &sum{-1*x} <= 0. "
                      "b :- &sum{1*x} <= 5. "
                      ":- not a. "
                      ":- not b.") == S({"a b x=0", "a b x=1", "a b x=2", "a b x=3", "a b x=4", "a b x=5"}));
        REQUIRE(solve("&sum { 1 * x + (-1) * y } <= -1. "
                      "&sum { 1 * y + (-1) * x } <= -1.",
                      -20, 20) == S({}));
        REQUIRE(solve("&sum { 1 } <= 2. ") == S({""}));
        REQUIRE(solve("&sum { 2 } <= 1. ").empty());
        REQUIRE(solve("{a}. "
                      "&sum {   1 *x } <= -5 :- a. "
                      "&sum { (-1)*x } <= -5 :- not a.",
                      -6, 6) == S({"a x=-5", "a x=-6", "x=5", "x=6"}));
        REQUIRE(solve("{a}. &sum { x } >= 3 :- a. &sum { x } <= 0 :- not a.", 0, 3) == S({"a x=3", "x=0"}));
        REQUIRE(solve("{a}. &sum { x } != 5 :- not a. &dom {5; 10} = x.", 0, 10) == S({"a x=10", "a x=5", "x=10"}));
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
        REQUIRE(solve("&sum { 2*(x+x) } <= 4.", 0, 3) == S({"x=0", "x=1"}));
        REQUIRE(solve("&sum { (x+x)*2 } <= 4.", 0, 3) == S({"x=0", "x=1"}));
        REQUIRE(solve("a :- &sum { x } >= 1.", -3, 3) == S({"a x=1", "a x=2", "a x=3", "x=-1", "x=-2", "x=-3", "x=0"}));
        REQUIRE(solve("a :- &sum { x } = 1.", -3, 3) == S({"a x=1", "x=-1", "x=-2", "x=-3", "x=0", "x=2", "x=3"}));
        REQUIRE(solve("&sum { 5*x + 10*y } = 20.", -3, 3) == S({"x=-2 y=3", "x=0 y=2", "x=2 y=1"}));
        REQUIRE(solve("&sum { -5*x + 10*y } = 20.", -3, 3) == S({"x=-2 y=1", "x=0 y=2", "x=2 y=3"}));
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
    SECTION("shift") {
        REQUIRE(solve("{a}. :- a, &sum { x } < 3. :- not a, &sum { x } > 0.", 0, 3) == S({"a x=3", "x=0"}));
    }
    SECTION("bug shift") { REQUIRE(solve("{a}. b. c :- a: b.", 0, 3) == S({"a b c", "b"})); }
    SECTION("show") {
        REQUIRE(solve("&sum { p(X) } = 0 :- X=1..3. &sum { q(X) } = 0 :- X=1..3. &show { }.") == S({""}));
        REQUIRE(solve("&sum { p(X) } = 0 :- X=1..3. &sum { q(X) } = 0 :- X=1..3. &show { p/1 }.") ==
                S({"p(1)=0 p(2)=0 p(3)=0"}));
        REQUIRE(solve("&sum { p(X) } = 0 :- X=1..3. &sum { q(X) } = 0 :- X=1..3. &show { p(1); p(2); q(1) }.") ==
                S({"p(1)=0 p(2)=0 q(1)=0"}));
        REQUIRE(solve("&sum { p(X) } = 0 :- X=1..3. &sum { q(X) } = 0 :- X=1..3. &show { p/1; q(1) }.") ==
                S({"p(1)=0 p(2)=0 p(3)=0 q(1)=0"}));
    }
    SECTION("set") {
        REQUIRE(solve("&sum { 1;1 } = x.") == S({"x=2"}));
        REQUIRE(solve("&sum { 1 : X=1..3 } = x.") == S({"x=3"}));
        REQUIRE(solve("&sum { 1 : X=(1;2;3) } = x.") == S({"x=3"}));
        REQUIRE(solve("&sum { 1 : 1=(X;Y;Z) } = x.") == S({"x=3"}));
        REQUIRE(solve("&sum { v(X) } = X :- X=1..3. &sum { v(X) : X=1..2; v(X) : X=2..3 } = x.") ==
                S({"x=8 v(1)=1 v(2)=2 v(3)=3"}));
    }
    SECTION("equality in a body") {
        // Guessing both directions of the equality false activates two
        // contradictory bounds, and propagating them walks through the
        // domain one value at a time. The parser rules that guess out, so
        // the number of order atoms stays small on a large domain.
        Propagator p;
        p.config() = create_configs(-100000, 100000).front();
        SolveEventHandler handler{p};
        Clingo::Control ctl{{"1", "-t1"}};
        ctl.add("base", {}, THEORY);
        std::string prg = "a :- &sum { x; y } = 4.";
        Clingo::AST::with_builder(ctl, [&prg](Clingo::AST::ProgramBuilder &builder) -> void {
            Clingo::AST::parse_string(prg.c_str(), [&builder](Clingo::AST::Node const &stm) -> void {
                transform(stm, [&builder](Clingo::AST::Node const &stm) -> void { builder.add(stm); }, true);
            });
        });
        ctl.register_propagator(p);
        ctl.ground({{"base", {}}});
        REQUIRE(ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_satisfiable());
        auto introduced =
            ctl.statistics()["user_step"]["Clingcon"]["Thread"][static_cast<size_t>(0)]["Literals introduced"].value();
        REQUIRE(introduced < 1000);
    }
    SECTION("string") {
        Propagator p;
        SolveEventHandler handler{p};
        Clingo::Control ctl{{"100"}};
        ctl.add("base", {}, THEORY);
        Clingo::AST::with_builder(ctl, [](Clingo::AST::ProgramBuilder &builder) -> void {
            Clingo::AST::parse_string(
                R"(&sum { ("a\"b\\c",0) } = 2.)", [&builder](Clingo::AST::Node const &stm) -> void {
                    transform(stm, [&builder](Clingo::AST::Node const &stm) -> void { builder.add(stm); }, true);
                });
        });
        ctl.register_propagator(p);
        ctl.ground({{"base", {}}});
        {
            auto hnd = ctl.solve(Clingo::LiteralSpan{}, &handler, false, true);
            for (auto &&mdl : hnd) {
                auto syms = mdl.symbols(Clingo::ShowType::Theory);
                REQUIRE(syms.size() == 1);
                REQUIRE(syms.front() == Clingo::Function("__csp", {Clingo::Function("", {Clingo::String(R"(a"b\c)"),
                                                                                         Clingo::Number(0)}),
                                                                   Clingo::Number(2)}));
            }
        }
    }
}

namespace {

// Solves a program on one thread with the given domain and returns whether
// it is satisfiable and how many order atoms the solve call introduced.
auto solve_counting(std::string const &prg, val_t min_int, val_t max_int) -> std::pair<bool, double> {
    Propagator p;
    p.config() = create_configs(min_int, max_int).front();
    SolveEventHandler handler{p};
    Clingo::Control ctl{{"1", "-t1"}};
    ctl.add("base", {}, THEORY);
    Clingo::AST::with_builder(ctl, [&prg](Clingo::AST::ProgramBuilder &builder) -> void {
        Clingo::AST::parse_string(prg.c_str(), [&builder](Clingo::AST::Node const &stm) -> void {
            transform(stm, [&builder](Clingo::AST::Node const &stm) -> void { builder.add(stm); }, true);
        });
    });
    ctl.register_propagator(p);
    ctl.ground({{"base", {}}});
    auto satisfiable = ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_satisfiable();
    auto introduced =
        ctl.statistics()["user_step"]["Clingcon"]["Thread"][static_cast<size_t>(0)]["Literals introduced"].value();
    return {satisfiable, introduced};
}

} // namespace

TEST_CASE("bounds", "[solving]") {
    // Two constraints over the same expression whose bounds cannot both hold
    // get a clause when they are created, so the search never propagates the
    // two of them against each other through a large domain.
    constexpr char const *PAIR = "l1 :- &sum { x; y } <= 4. l2 :- &sum { x; y } >= 4. ";
    SECTION("contradictory bounds from a guess") {
        auto [satisfiable, introduced] = solve_counting(std::string{PAIR} + ":- l1.", -100000, 100000);
        REQUIRE(satisfiable);
        REQUIRE(introduced < 1000);
    }
    SECTION("contradictory bounds forced") {
        auto [satisfiable, introduced] = solve_counting(std::string{PAIR} + ":- l1. :- l2.", -100000, 100000);
        REQUIRE(!satisfiable);
        REQUIRE(introduced < 1000);
    }
    SECTION("contradictory facts") {
        auto [satisfiable, introduced] = solve_counting("&sum { x } < y. &sum { y } < x.", -100000, 100000);
        REQUIRE(!satisfiable);
        REQUIRE(introduced < 1000);
    }
    SECTION("proportional coefficients") {
        auto [satisfiable, introduced] =
            solve_counting("l1 :- &sum { 2*x; 2*y } <= 8. l2 :- &sum { 3*x; 3*y } >= 12. :- l1.", -100000, 100000);
        REQUIRE(satisfiable);
        REQUIRE(introduced < 1000);
    }
    SECTION("models are unchanged") {
        REQUIRE(solve("&dom { 0..2 } = x. &dom { 0..2 } = y. l1 :- &sum { x; y } <= 1. l2 :- &sum { x; y } >= 2.") ==
                S({"l1 x=0 y=0", "l1 x=0 y=1", "l1 x=1 y=0", "l2 x=0 y=2", "l2 x=1 y=1", "l2 x=1 y=2", "l2 x=2 y=0",
                   "l2 x=2 y=1", "l2 x=2 y=2"}));
    }
    SECTION("multi-shot") {
        // The constraint of the second step contradicts the one of the first.
        auto result = solve_multi(create_configs(0, 10).front(),
                                  "#program base. &dom { 0..10 } = x. &dom { 0..10 } = y. &sum { x } = 3. "
                                  "l1 :- &sum { x; y } <= 4. :- l1. "
                                  "#program step. l2 :- &sum { x; y } >= 4. :- l2.",
                                  {{"base", {}}, {"step", {}}});
        REQUIRE(result.size() == 10);
        REQUIRE(result.back() == "---");
    }
}

namespace {

// What a solve call on one thread reports besides its models.
struct CountReport {
    bool satisfiable{false};
    double literals{0}; //!< the "Literals introduced" statistic
    double cycles{0};   //!< the "Propagation cycles" statistic
};

// Solves a program on one thread with the given domain and clasp options.
auto solve_report(std::string const &prg, val_t min_int, val_t max_int, std::vector<char const *> const &opts = {})
    -> CountReport {
    Propagator p;
    p.config() = create_configs(min_int, max_int).front();
    SolveEventHandler handler{p};
    std::vector<char const *> args{"1", "-t1"};
    args.insert(args.end(), opts.begin(), opts.end());
    Clingo::Control ctl{args};
    ctl.add("base", {}, THEORY);
    Clingo::AST::with_builder(ctl, [&prg](Clingo::AST::ProgramBuilder &builder) -> void {
        Clingo::AST::parse_string(prg.c_str(), [&builder](Clingo::AST::Node const &stm) -> void {
            transform(stm, [&builder](Clingo::AST::Node const &stm) -> void { builder.add(stm); }, true);
        });
    });
    ctl.register_propagator(p);
    ctl.ground({{"base", {}}});
    CountReport report;
    report.satisfiable = ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_satisfiable();
    auto thread = ctl.statistics()["user_step"]["Clingcon"]["Thread"][static_cast<size_t>(0)];
    report.literals = thread["Literals introduced"].value();
    report.cycles = thread["Propagation cycles"].value();
    return report;
}

} // namespace

TEST_CASE("cycles", "[solving]") {
    // Constraints that tighten each other's bounds round after round walk
    // through the domains one value at a time. The solver follows the
    // origins of the bounds, finds the cycle, and adds a conflicting clause.
    constexpr val_t WIDE = 1000000;
    constexpr char const *CHAIN = "&sum { x } <= y - 1. &sum { y } <= z - 1. ";
    SECTION("chain of differences forced") {
        auto report = solve_report(std::string{CHAIN} + "&sum { z } <= x - 1.", -WIDE, WIDE);
        REQUIRE(!report.satisfiable);
        REQUIRE(report.cycles >= 1);
    }
    SECTION("chain of differences from a guess") {
        auto report = solve_report(std::string{CHAIN} + "l :- &sum { z } >= x.", -WIDE, WIDE);
        REQUIRE(report.satisfiable);
        REQUIRE(report.cycles >= 1);
        REQUIRE(report.literals < 1000);
    }
    SECTION("guess kept by a domain heuristic") {
        auto report = solve_report(std::string{CHAIN} + "l :- &sum { z } <= x - 1. #heuristic l. [1, true]", -WIDE,
                                   WIDE, {"--heuristic=Domain"});
        REQUIRE(report.satisfiable);
        REQUIRE(report.cycles >= 1);
        REQUIRE(report.literals < 1000);
    }
    SECTION("loop through sums forced") {
        auto report = solve_report("&sum { x; y } <= 3. &sum { y; z } >= 5. &sum { x } >= z.", -WIDE, WIDE);
        REQUIRE(!report.satisfiable);
        REQUIRE(report.cycles >= 1);
    }
    SECTION("loop through sums from a guess") {
        auto report = solve_report("&sum { x; y } <= 3. &sum { y; z } >= 5. l :- &sum { x } < z.", -WIDE, WIDE);
        REQUIRE(report.satisfiable);
        REQUIRE(report.cycles >= 1);
        REQUIRE(report.literals < 1000);
    }
    SECTION("unit gain with coefficients") {
        // The slopes 1/2, 1 and 2 multiply to one, and no two constraints
        // share an expression.
        auto report = solve_report("&sum { 2*x } <= y. &sum { y } <= z. &sum { z } <= 2*x - 1.", -WIDE, WIDE);
        REQUIRE(!report.satisfiable);
        REQUIRE(report.cycles >= 1);
    }
    SECTION("gain above one with a bound") {
        // The bounds move geometrically, so the conflict is found in a few
        // rounds without the cycle check.
        auto report = solve_report("&sum { x } <= 2*y - 1. &sum { y } <= x. &sum { x } <= 0.", -WIDE, WIDE);
        REQUIRE(!report.satisfiable);
    }
    SECTION("gain above one keeps its models") {
        REQUIRE(solve("&dom { -5..5 } = x. &dom { -5..5 } = y. &sum { x } <= 2*y - 1. &sum { y } <= x.") ==
                S({"x=1 y=1", "x=2 y=2", "x=3 y=2", "x=3 y=3", "x=4 y=3", "x=4 y=4", "x=5 y=3", "x=5 y=4", "x=5 y=5"}));
    }
    SECTION("converging loop keeps its models") {
        REQUIRE(solve("&dom { 0..4 } = x. &dom { 0..4 } = y. &sum { 2*x } <= y. &sum { y } <= x.") == S({"x=0 y=0"}));
    }
    SECTION("guessed loop keeps its models") {
        auto models = solve("&dom { 0..5 } = x. &dom { 0..5 } = y. &dom { 0..5 } = z. "
                            "&sum { x } <= y - 1. &sum { y } <= z - 1. l :- &sum { z } >= x.");
        REQUIRE(models.size() == 20);
        for (auto const &model : models) {
            REQUIRE(model.rfind("l ", 0) == 0);
        }
    }
    SECTION("multi-shot") {
        // The constraint of the second step closes the cycle.
        auto result = solve_multi(create_configs(0, 10).front(),
                                  "#program base. &dom { 0..10 } = a. &dom { 0..10 } = b. &dom { 0..10 } = c. "
                                  "&dom { 0..10 } = d. &dom { 0..10 } = e. &sum { a } = 0. &sum { e } = 4. "
                                  "&sum { a } <= b - 1. &sum { b } <= c - 1. &sum { c } <= d - 1. &sum { d } <= e - 1. "
                                  "#program step. &sum { e } <= a - 1.",
                                  {{"base", {}}, {"step", {}}});
        REQUIRE(result == S({"a=0 b=1 c=2 d=3 e=4", "---"}));
    }
}

TEST_CASE("nsum", "[solving]") {
    SECTION("simple") {
        REQUIRE(solve("&dom { 1..2 } = a.\n"
                      "&dom { 1..2 } = b.\n"
                      "&dom { -5..5 } = c.\n"
                      "&nsum { a*b } = c.\n") == S({"a=1 b=1 c=1", "a=1 b=2 c=2", "a=2 b=1 c=2", "a=2 b=2 c=4"}));
        REQUIRE(solve("&dom { -2..2 } = a.\n"
                      "&dom { -2..2 } = b.\n"
                      "&dom { -5..5 } = c.\n"
                      "&nsum { a*b } = c.\n") ==
                S({"a=-1 b=-1 c=1", "a=-1 b=-2 c=2", "a=-1 b=0 c=0", "a=-1 b=1 c=-1", "a=-1 b=2 c=-2",
                   "a=-2 b=-1 c=2", "a=-2 b=-2 c=4", "a=-2 b=0 c=0", "a=-2 b=1 c=-2", "a=-2 b=2 c=-4",
                   "a=0 b=-1 c=0",  "a=0 b=-2 c=0",  "a=0 b=0 c=0",  "a=0 b=1 c=0",   "a=0 b=2 c=0",
                   "a=1 b=-1 c=-1", "a=1 b=-2 c=-2", "a=1 b=0 c=0",  "a=1 b=1 c=1",   "a=1 b=2 c=2",
                   "a=2 b=-1 c=-2", "a=2 b=-2 c=-4", "a=2 b=0 c=0",  "a=2 b=1 c=2",   "a=2 b=2 c=4"}));
    }
}

TEST_CASE("multishot", "[solving]") {
    SECTION("simple") {
        REQUIRE(solve_multi("#program a.\n"
                            "&dom{ 1..2 } = a.\n"
                            "#program b.\n"
                            "{ b }.\n"
                            "&dom{ 1..1 } = b.\n",
                            {{"a", {}}, {"b", {}}}) ==
                S{{"a=1", "a=2", "---", "a=1 b=1", "a=2 b=1", "b a=1 b=1", "b a=2 b=1"}});
    }
    SECTION("enumerate") {
        REQUIRE(solve_multi("#program prog(id).\n"
                            "{selected(id)}.\n"
                            "&dom{ 1..2 } = val(id).\n",
                            {{"prog", {Clingo::Function("a", {})}}, {"prog", {Clingo::Function("b", {})}}}) ==
                S{{"selected(a) val(a)=1",
                   "selected(a) val(a)=2",
                   "val(a)=1",
                   "val(a)=2",
                   "---",
                   "selected(a) selected(b) val(a)=1 val(b)=1",
                   "selected(a) selected(b) val(a)=1 val(b)=2",
                   "selected(a) selected(b) val(a)=2 val(b)=1",
                   "selected(a) selected(b) val(a)=2 val(b)=2",
                   "selected(a) val(a)=1 val(b)=1",
                   "selected(a) val(a)=1 val(b)=2",
                   "selected(a) val(a)=2 val(b)=1",
                   "selected(a) val(a)=2 val(b)=2",
                   "selected(b) val(a)=1 val(b)=1",
                   "selected(b) val(a)=1 val(b)=2",
                   "selected(b) val(a)=2 val(b)=1",
                   "selected(b) val(a)=2 val(b)=2",
                   "val(a)=1 val(b)=1",
                   "val(a)=1 val(b)=2",
                   "val(a)=2 val(b)=1",
                   "val(a)=2 val(b)=2"}});
    }
    SECTION("optimize") {
        REQUIRE(solve_opt("#program base. "
                          "&dom {-3..9} = x. "
                          "&minimize { x }. "
                          "#program next. "
                          "&sum { x } >= 5.",
                          {{"base", {}}, {"next", {}}}) == O({-3, 5}));
    }
}
