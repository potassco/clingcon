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

#include <clingcon/propagator.hh>
#include <clingcon/parsing.hh>
#include "catch.hpp"

#include <iostream>

using namespace Clingcon;

class SolveEventHandler : public Clingo::SolveEventHandler {
public:
    bool on_model(Clingo::Model &model) override {
        std::cerr << model << std::endl;
        return true;
    }
};

TEST_CASE("solving", "[solving]") {
    SECTION("simple") {
        SolveEventHandler handler;
        Propagator p;
        p.config().default_solver_config.refine_introduce = false;
        p.config().default_solver_config.refine_reasons = false;
        p.config().default_solver_config.propagate_chain = false;
        Clingo::Control ctl{{"10"}};
        ctl.add("base", {}, THEORY);
        ctl.with_builder([](Clingo::ProgramBuilder &builder) {
            Clingo::parse_program("&sum{ x } > 0. &sum{ x } < 3.", [&builder](Clingo::AST::Statement &&stm) {
                transform(std::move(stm), [&builder](Clingo::AST::Statement &&stm) {
                    builder.add(stm);
                }, true);
            });
        });
        ctl.register_propagator(p);
        ctl.ground({{"base", {}}});
        auto ret = ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get();
        std::cerr << "ret: " << ret << std::endl;
    }
}
