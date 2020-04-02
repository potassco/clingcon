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

#include "clingcon.h"
#include "clingcon/propagator.hh"
#include "clingcon/parsing.hh"

#include <clingo.hh>
#include <stdexcept>

#define CLINGCON_TRY try // NOLINT
#define CLINGCON_CATCH catch (...){ Clingo::Detail::handle_cxx_error(); return false; } return true // NOLINT

using Clingo::Detail::handle_error;

using namespace Clingcon;

struct clingcon_theory {
    Propagator propagator;
    bool shift_constraints{true};
};

namespace {

bool init(clingo_propagate_init_t* c_init, void* data) {
    CLINGCON_TRY {
        Clingo::PropagateInit init{c_init};
        static_cast<Propagator*>(data)->init(init);
    }
    CLINGCON_CATCH;
}

bool propagate(clingo_propagate_control_t* c_ctl, const clingo_literal_t *changes, size_t size, void* data) {
    CLINGCON_TRY {
        Clingo::PropagateControl ctl{c_ctl};
        static_cast<Propagator*>(data)->propagate(ctl, {changes, size});
    }
    CLINGCON_CATCH;
}

void undo(clingo_propagate_control_t const *c_ctl, clingo_literal_t const *changes, size_t size, void* data) {
    Clingo::PropagateControl ctl(const_cast<clingo_propagate_control_t *>(c_ctl)); // NOLINT
    static_cast<Propagator*>(data)->undo(ctl, {changes, size});
}

bool check(clingo_propagate_control_t *c_ctl, void* data) {
    CLINGCON_TRY {
        Clingo::PropagateControl ctl{c_ctl};
        static_cast<Propagator*>(data)->check(ctl);
    }
    CLINGCON_CATCH;
}

} // namespace

extern "C" bool clingcon_create(clingcon_theory_t **theory) {
    CLINGCON_TRY {
        *theory = new clingcon_theory(); // NOLINT
    }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_register(clingcon_theory_t *theory, clingo_control_t* control) {
    static clingo_propagator_t propagator = { init, propagate, undo, check, nullptr };
    return
        clingo_control_add(control, "base", nullptr, 0, Clingcon::THEORY) &&
        clingo_control_register_propagator(control, &propagator, &theory->propagator, false);
}

extern "C" bool clingcon_rewrite_statement(clingcon_theory_t *theory, clingo_ast_statement_t const *stm, clingcon_rewrite_callback_t add, void *data) {
    CLINGCON_TRY {
        Clingo::StatementCallback cb = [&](Clingo::AST::Statement &&stm) {
            transform(std::move(stm), [add, data](Clingo::AST::Statement &&stm){
                Clingo::AST::Detail::ASTToC visitor;
                auto x = stm.data.accept(visitor);
                x.location = stm.location;
                handle_error(add(&x, data));
            }, theory->shift_constraints);
        };
        Clingo::AST::Detail::convStatement(stm, cb);
    }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_prepare(clingcon_theory_t *theory, clingo_control_t* control) {
    static_cast<void>(theory);
    static_cast<void>(control);
    return true;
}

extern "C" bool clingcon_destroy(clingcon_theory_t *theory) {
    delete theory; // NOLINT
    return true;
}

extern "C" bool clingcon_configure(clingcon_theory_t *theory, char const *key, char const *value) {
    static_cast<void>(theory);
    static_cast<void>(key);
    static_cast<void>(value);
    throw std::runtime_error("implement me!!!");
}

extern "C" bool clingcon_register_options(clingcon_theory_t *theory, clingo_options_t* options) {
    static_cast<void>(theory);
    static_cast<void>(options);
    throw std::runtime_error("implement me!!!");
}

extern "C" bool clingcon_validate_options(clingcon_theory_t *theory) {
    static_cast<void>(theory);
    throw std::runtime_error("implement me!!!");
}

extern "C" bool clingcon_on_model(clingcon_theory_t *theory, clingo_model_t* model) {
    CLINGCON_TRY {
        Clingo::Model m{model};
        theory->propagator.on_model(m);
    }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_lookup_symbol(clingcon_theory_t *theory, clingo_symbol_t symbol, size_t *index) {
    if (auto var = theory->propagator.get_index(Clingo::Symbol{symbol}); var.has_value()) {
        *index = *var + 1;
        return true;
    }
    return false;
}

extern "C" clingo_symbol_t clingcon_get_symbol(clingcon_theory_t *theory, size_t index) {
    auto sym = theory->propagator.get_symbol(index - 1);
    assert(sym.has_value());
    return sym->to_c();
}

extern "C" void clingcon_assignment_begin(clingcon_theory_t *theory, uint32_t thread_id, size_t *index) {
    static_cast<void>(theory);
    static_cast<void>(thread_id);
    *index = 0;
}

extern "C" bool clingcon_assignment_next(clingcon_theory_t *theory, uint32_t thread_id, size_t *index) {
    static_cast<void>(thread_id);
    auto const &map = theory->propagator.var_map();
    auto it = map.lower_bound(*index);
    if (it != map.end()) {
        *index = *index + 1;
        return true;
    }
    return false;
}

extern "C" bool clingcon_assignment_has_value(clingcon_theory_t *theory, uint32_t thread_id, size_t index) {
    static_cast<void>(thread_id);
    return theory->propagator.get_symbol(index - 1).has_value();
}

extern "C" void clingcon_assignment_get_value(clingcon_theory_t *theory, uint32_t thread_id, size_t index, clingcon_value_t *value) {
    value->type = clingcon_value_type_int; // NOLINT
    value->int_number = theory->propagator.get_value(index - 1, thread_id); // NOLINT
}

extern "C" bool clingcon_on_statistics(clingcon_theory_t *theory, clingo_statistics_t* step, clingo_statistics_t* accu) {
    uint64_t step_root, accu_root; // NOLINT
    if (!clingo_statistics_root(step, &step_root) || !clingo_statistics_root(accu, &accu_root)) {
        return false;
    }
    CLINGCON_TRY {
        Clingo::UserStatistics step_stats{step, step_root};
        Clingo::UserStatistics accu_stats{accu, accu_root};
        theory->propagator.on_statistics(step_stats, accu_stats);
    }
    CLINGCON_CATCH;
}
