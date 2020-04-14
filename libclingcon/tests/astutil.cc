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

#include "clingcon/astutil.hh"
#include "catch.hpp"
#include <sstream>

using namespace Clingcon;

using svec = std::vector<std::string>;
using pvec = std::vector<std::vector<int>>;

svec collect(char const *prg) {
    VarSet vars;
    Clingo::parse_program(prg, [&](Clingo::AST::Statement &&stm) { collect_variables(vars, stm); });
    return {vars.begin(), vars.end()};
}

pvec product(pvec const &vec) {
    pvec ret;
    cross_product(vec, [&](auto begin, auto end) { ret.emplace_back(begin, end); });
    return ret;
}

std::string unpool(char const *prg) {
    bool sep = false;
    std::stringstream oss;
    Clingo::parse_program(prg, [&](Clingo::AST::Statement &&stm) {
        if (!stm.data.is<Clingo::AST::Program>()) {
            unpool(std::move(stm), [&](Clingo::AST::Statement &&stm) {
                oss << (sep ? "\n" : "") << stm;
                sep = true;
            });
        }
    });
    return oss.str();
}

TEST_CASE("astutil", "[astutil]") {
    SECTION("collect") {
        REQUIRE(collect("p(X) :- &p{ Y }.") == svec({"X", "Y"}));
        REQUIRE(collect("p(X) :- p(Y).") == svec({"X", "Y"}));
        REQUIRE(collect("#show p(X) : p(Y).") == svec({"X", "Y"}));
        REQUIRE(collect("#external p(X) : p(Y).") == svec({"X", "Y"}));
    }
    SECTION("product") {
        REQUIRE(product({}) == pvec({{}}));
        REQUIRE(product({{}}) == pvec({}));
        REQUIRE(product({{1,2}, {3}, {4,5}}) == pvec({{1, 3, 4}, {1, 3, 5}, {2, 3, 4}, {2, 3, 5}}));
    }
    SECTION("unpool") {
        REQUIRE(unpool("&p(1;2) { }.") == "&p(1) {  }.\n&p(2) {  }.");
        REQUIRE(unpool("&p { X : p(1;2) }.") == "&p { X : p(1); X : p(2) }.");
        REQUIRE(unpool("&p { X : p(1;2), p(3;4) }.") == "&p { X : p(1),p(3); X : p(1),p(4); X : p(2),p(3); X : p(2),p(4) }.");
        REQUIRE(unpool("&p { X : 1+(1;2)<3 }.") == "&p { X : (1+1)<3; X : (1+2)<3 }.");
        REQUIRE(unpool("&p(1;2) { X : p(1;2) } :- &q(1;2) { X : p(1;2) }, not &r(1;2) { X : p(1;2) }.") ==
            "&p(1) { X : p(1); X : p(2) } :- &q(1) { X : p(1); X : p(2) }; not &r(1) { X : p(1); X : p(2) }.\n"
            "&p(1) { X : p(1); X : p(2) } :- &q(1) { X : p(1); X : p(2) }; not &r(2) { X : p(1); X : p(2) }.\n"
            "&p(1) { X : p(1); X : p(2) } :- &q(2) { X : p(1); X : p(2) }; not &r(1) { X : p(1); X : p(2) }.\n"
            "&p(1) { X : p(1); X : p(2) } :- &q(2) { X : p(1); X : p(2) }; not &r(2) { X : p(1); X : p(2) }.\n"
            "&p(2) { X : p(1); X : p(2) } :- &q(1) { X : p(1); X : p(2) }; not &r(1) { X : p(1); X : p(2) }.\n"
            "&p(2) { X : p(1); X : p(2) } :- &q(1) { X : p(1); X : p(2) }; not &r(2) { X : p(1); X : p(2) }.\n"
            "&p(2) { X : p(1); X : p(2) } :- &q(2) { X : p(1); X : p(2) }; not &r(1) { X : p(1); X : p(2) }.\n"
            "&p(2) { X : p(1); X : p(2) } :- &q(2) { X : p(1); X : p(2) }; not &r(2) { X : p(1); X : p(2) }.");
    }
}
