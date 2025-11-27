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

std::string const ENC = R"(
#show packed/2.
bin(1..3).
capacity(1, 10).
capacity(2, 7).
capacity(3, 5).

item(1..5).
size(1,3).
size(2,5).
size(3,4).
size(4,3).
size(5,2).

{ packed(I,B) : bin(B) } = 1 :- item(I).

&sum{ load(I,B) } = S :- packed(I,B), size(I,S).
&sum{ load(I,B) } = 0 :- item(I), bin(B), not packed(I,B).

&sum{ load(I,B) : item(I) } <= C :- capacity(B,C).

usedBins(U) :- U = #count{ B : packed(_,B) }.
#minimize{ U : usedBins(U) }.
%#minimize{ 1,B : packed(_,B) }.
)";

struct AFixture {

    [[nodiscard]] auto packed(int a, int b) const {
        auto atom = ctl.base().get(Clingo::Function(lib, "packed", {Clingo::Number(a), Clingo::Number(b)}));
        return atom.value().literal(); // NOLINT
    }

    [[nodiscard]] auto step(Clingo::ProgramLiteralVector assumptions = {}) const {
        SolveEventHandler hnd{*prp};
        if (ctl.solve(assumptions, std::ref(hnd)).interrupted()) {
            throw std::runtime_error("interrupted");
        }
        std::ranges::sort(hnd.models);
        return std::move(hnd.models);
    }

    [[nodiscard]] auto bound() const {
        auto stat = ctl.stats()["summary"].map();
        REQUIRE(stat.contains("costs"));
        return static_cast<int>(stat["costs"][0].value());
    }

    Clingo::Library lib;
    Clingo::Control ctl{lib, {"0"}};
    Propagator *prp = &ctl.register_propagator(std::make_unique<Propagator>(lib));
};

} // namespace

TEST_CASE_METHOD(AFixture, "assumptions", "[assumptions]") {
    ctl.parse_string(THEORY);
    auto prg = Clingo::AST::Program{lib};
    Clingo::AST::parse(lib, ENC, [&](auto const &stm) {
        Clingcon::transform(lib, stm, [&](Clingo::AST::Node const &stm) { prg.add(stm); }, true);
    });
    ctl.join(prg);

    ctl.ground({{"base", {}}});
    REQUIRE(!step().empty());
    REQUIRE(bound() == 2);

    // ensure it is unsatisfiable
    ctl.config()["solve"]["opt_mode"] = "opt,1";
    REQUIRE(step({packed(4, 1), packed(1, 1)}).empty());

    // ensure it is unsatisfiable
    ctl.config()["solve"]["opt_mode"] = "opt,1";
    REQUIRE(step({packed(2, 2), packed(3, 1), packed(4, 1), packed(1, 1)}).empty());
}
