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

using namespace Clingcon;

using vec = std::vector<std::string>;

std::vector<std::string> collect(char const *prg) {
    std::set<char const*> vars;
    Clingo::parse_program(prg, [&](Clingo::AST::Statement &&stm) { collect_variables(vars, stm); });
    return {vars.begin(), vars.end()};
}

TEST_CASE("astutil", "[astutil]") {
    SECTION("collect") {
        REQUIRE(collect("p(X) :- &p{ Y }.") == vec({"X", "Y"}));
        REQUIRE(collect("p(X) :- p(Y).") == vec({"X", "Y"}));
        REQUIRE(collect("#show p(X) : p(Y).") == vec({"X", "Y"}));
        REQUIRE(collect("#external p(X) : p(Y).") == vec({"X", "Y"}));
    }
}

