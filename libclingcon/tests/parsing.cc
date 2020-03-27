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

#include "clingcon/parsing.hh"
#include "catch.hpp"

using namespace Clingcon;

using sret = std::pair<CoVarVec, val_t>;

sret simplify(CoVarVec const &vec, bool drop_zero=true) {
    CoVarVec ret = vec;
    auto rhs = Clingcon::simplify(ret, drop_zero);
    return {ret, rhs};
}

std::string transform(char const *prg, bool shift=true) {
    bool sep = false;
    std::ostringstream oss;
    Clingo::parse_program(prg, [&](Clingo::AST::Statement &&stm) {
        if (!stm.data.is<Clingo::AST::Program>()) {
            transform(std::move(stm), [&](Clingo::AST::Statement &&stm) {
                oss << (sep ? "\n" : "") << stm;
                sep = true;
            }, shift);
        }
    });
    return oss.str();
}

TEST_CASE("parsing", "[parsing]") {
    SECTION("simplify") {
        REQUIRE(simplify({}) == sret({}, 0));
        REQUIRE(simplify({{1, 0}, {1, 1}}) == sret({{1, 0}, {1, 1}}, 0));
        REQUIRE(simplify({{1, INVALID_VAR}}) == sret({}, -1));
        REQUIRE(simplify({{0, 0}}) == sret({}, 0));
        REQUIRE(simplify({{0, 0}, {0, 0}}, false) == sret({{0, 0}}, 0));
        REQUIRE(simplify({{0, 0}, {1, INVALID_VAR}, {2, INVALID_VAR}, {3, 0}, {4, 0}}) == sret({{7, 0}}, -3));

        REQUIRE_THROWS_AS(simplify({{std::numeric_limits<int>::max(), 0}, {std::numeric_limits<int>::max(), 0}}), std::overflow_error const &);
        REQUIRE_THROWS_AS(simplify({{std::numeric_limits<int>::min(), INVALID_VAR}}), std::overflow_error const &);
    }
    SECTION("transform") {
        REQUIRE(transform("&sum{ } = 0 :- &sum{ } = 1.") == "&__sum_h {  } = 0 :- &__sum_b {  } = 1.");
        REQUIRE(transform(":- &sum{ } = 0.") == "&__sum_h {  } != 0.");
        REQUIRE(transform(":- &sum{ } = 0, &sum{ } = 1.") == "&__sum_h {  } != 0 :- &__sum_b {  } = 1.");

        REQUIRE(transform("&sum{ X } = 0.") == "&__sum_h { X :  } = 0.");
        REQUIRE(transform("&sum{ X : p(X,Y) } = 0.") == "&__sum_h { X,Y : p(X,Y) } = 0.");
        REQUIRE(transform("&sum{ X : p(X,Y); X : q(X,Y) } = 0.") == "&__sum_h { X,0,Y : p(X,Y); X,1,Y : q(X,Y) } = 0.");
        REQUIRE(transform("&sum{ X : p(X,_) } = 0.") == "&__sum_h { X : p(X,_) } = 0.");
    }
}
