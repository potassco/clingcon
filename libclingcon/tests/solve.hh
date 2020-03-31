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

#ifndef CLINGCON_TEST_SOLVE_H
#define CLINGCON_TEST_SOLVE_H

#include <clingcon/propagator.hh>
#include <clingcon/parsing.hh>

#include <sstream>

using namespace Clingcon;

using S = std::vector<std::string>;

class SolveEventHandler : public Clingo::SolveEventHandler {
public:
    SolveEventHandler(Propagator &p) : p{p} { }
    bool on_model(Clingo::Model &model) override {
        std::ostringstream oss;
        bool sep = false;
        std::vector<Clingo::Symbol> symbols = model.symbols();
        std::sort(symbols.begin(), symbols.end());
        for (auto &sym : symbols) {
            if (sep) {
                oss << " ";
            }
            sep = true;
            oss << sym;
        }
        std::vector<std::pair<Clingo::Symbol, val_t>> assignment;
        for (auto [sym, var] : p.get_var_map()) {
            if (p.shown(var)) {
                assignment.emplace_back(sym, p.get_value(var, model.thread_id()));
            }
        }
        std::sort(assignment.begin(), assignment.end());
        for (auto [sym, val] : assignment) {
            if (sep) {
                oss << " ";
            }
            sep = true;
            oss << sym << "=" << val;
        }
        models.emplace_back(oss.str());
        return true;
    }
    S models;
    Propagator &p;
};

inline S solve(std::string const &prg, val_t min_int = Clingcon::DEFAULT_MIN_INT, val_t max_int = Clingcon::DEFAULT_MAX_INT) {
    Propagator p;
    SolveEventHandler handler{p};
    p.config().min_int = min_int;
    p.config().max_int = max_int;
    p.config().default_solver_config.refine_introduce = false;
    p.config().default_solver_config.refine_reasons = false;
    p.config().default_solver_config.propagate_chain = false;
    Clingo::Control ctl{{"100"}};
    ctl.add("base", {}, THEORY);
    ctl.with_builder([prg](Clingo::ProgramBuilder &builder) {
        Clingo::parse_program(prg.c_str(), [&builder](Clingo::AST::Statement &&stm) {
            transform(std::move(stm), [&builder](Clingo::AST::Statement &&stm) {
                builder.add(stm);
            }, true);
        });
    });
    ctl.register_propagator(p);
    ctl.ground({{"base", {}}});
    ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get();
    std::sort(handler.models.begin(), handler.models.end());

    return handler.models;
}

#endif // CLINGCON_TEST_SOLVE_H
