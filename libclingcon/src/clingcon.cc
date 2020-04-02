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
#include <sstream>
#include <map>

#define CLINGCON_TRY try // NOLINT
#define CLINGCON_CATCH catch (...){ Clingo::Detail::handle_cxx_error(); return false; } return true // NOLINT

using Clingo::Detail::handle_error;

using namespace Clingcon;

namespace {

constexpr uint32_t MAX_THREADS = 64;
enum class Target { RefineReasons, RefineIntroduce, PropagateChain, SplitAll };

} // namespace

struct clingcon_theory {
    Propagator propagator;
    Clingo::Detail::ParserList parsers;
    std::map<std::pair<Target, std::optional<uint32_t>>, bool> deferred;
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

char const *flag_str(bool value) {
    return value ? "yes" : "no";
}

template <typename... Args>
std::string format(Args &&... args) {
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args));
    return oss.str();
}

template<class T>
T strtonum(char const *value) {
    T ret = 0;
    bool sign = false;
    auto const *it = value;
    if (std::is_signed_v<T> && *it == '-') {
        sign = true;
        ++it; // NOLINT
    }
    if (!*it) {
        throw std::invalid_argument("integer expected");
    }
    for (; *it; ++it) { // NOLINT
        if ('0' <= *it && *it <= '9') {
            ret = safe_add<T>(safe_mul<T>(ret, 10), *it - '0'); // NOLINT
        }
        else {
            throw std::invalid_argument("integer expected");
        }
    }
    return sign ? safe_inv<T>(ret) : ret;
}

template<class T, T min=std::numeric_limits<T>::min(), T max=std::numeric_limits<T>::max()>
T parse_num(char const *value) {
    auto res = strtonum<T>(value);
    if (min <= res && res <= max) {
        return res;
    }
    throw std::invalid_argument("invalid argument");
}

template<class T, T min=std::numeric_limits<T>::min(), T max=std::numeric_limits<T>::max()>
std::function<bool (const char *)> parser_num(T &dest) {
    return [&dest](char const *value) {
        dest = parse_num<T>(value);
        return true;
    };
}

void set_value(Target target, SolverConfig &config, bool value) {
    switch (target) {
        case Target::RefineReasons: {
            config.refine_reasons = value;
            break;
        }
        case Target::RefineIntroduce: {
            config.refine_introduce = value;
            break;
        }
        case Target::PropagateChain: {
            config.propagate_chain = value;
            break;
        }
        case Target::SplitAll: {
            config.split_all = value;
            break;
        }
    }
}

void set_value(Target target, Config &config, std::pair<bool, std::optional<uint32_t>> value) {
    auto [flag, thread] = value;
    if (thread.has_value()) {
        set_value(target, config.solver_config(*thread), flag);
    }
    else {
        set_value(target, config.default_solver_config, flag);
        for (auto &sconf : config.solver_configs) {
            set_value(target, sconf, value.first);
        }
    }
}

bool parse_bool(char const *value, size_t len=6) { // NOLINT
    if (std::strncmp(value, "true", len) == 0 || std::strncmp(value, "yes", len) == 0 || std::strncmp(value, "1", len) == 0) {
        return true;
    }
    if (std::strncmp(value, "false", len) == 0 || std::strncmp(value, "no", len) == 0 || std::strncmp(value, "0", len) == 0) {
        return false;
    }
    throw std::invalid_argument("invalid argument");
}

std::pair<bool, std::optional<uint32_t>> parse_bool_thread(char const *value) {
    std::optional<uint32_t> thread = std::nullopt;
    size_t len = 0;
    char const *comma = std::strchr(value, ',');
    if (comma != nullptr) {
        thread = strtonum<uint32_t>(comma + 1); // NOLINT
        if (*thread >= MAX_THREADS) {
            throw std::invalid_argument("invalid argument");
        }
        len = comma - value;
    }
    else {
        len = std::strlen(value);
    }

    return {parse_bool(value, len), thread};

}

std::function<bool (const char *)> parser_bool_thread(clingcon_theory &theory, Target target) {
    return [&theory, target](char const *value) {
        auto [flag, thread] = parse_bool_thread(value);
        return theory.deferred.emplace(std::pair(target, thread), flag).second;
    };
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
    // Note: There is nothing todo.
    return true;
}

extern "C" bool clingcon_destroy(clingcon_theory_t *theory) {
    delete theory; // NOLINT
    return true;
}

extern "C" bool clingcon_configure(clingcon_theory_t *theory, char const *key, char const *value) {
    CLINGCON_TRY {
        auto config = theory->propagator.config();
        // translation
        if (std::strcmp(key, "shift-constraints") == 0) {
            theory->shift_constraints = parse_bool(value);
        }
        else if (std::strcmp(key, "sort-constraints") == 0) {
            config.sort_constraints = parse_bool(value);
        }
        else if (std::strcmp(key, "translate-clauses") == 0) {
            config.clause_limit = parse_num<uint32_t>(value);
        }
        else if (std::strcmp(key, "literals-only") == 0) {
            config.literals_only = parse_bool(value);
        }
        else if (std::strcmp(key, "translate-pb") == 0) {
            config.weight_constraint_limit = parse_num<uint32_t>(value);
        }
        else if (std::strcmp(key, "translate-distinct") == 0) {
            config.distinct_limit = parse_num<uint32_t>(value);
        }
        else if (std::strcmp(key, "translate-opt") == 0) {
            config.translate_minimize = parse_bool(value);
        }
        // hidden/debug
        else if (std::strcmp(key, "min-int") == 0) {
            config.min_int = parse_num<val_t, MIN_VAL, MAX_VAL>(value);
        }
        else if (std::strcmp(key, "max-int") == 0) {
            config.max_int = parse_num<val_t, MIN_VAL, MAX_VAL>(value);
        }
        else if (std::strcmp(key, "check-solution") == 0) {
            config.check_solution = parse_bool(value);
        }
        else if (std::strcmp(key, "check-state") == 0) {
            config.check_state = parse_bool(value);
        }
        // propagation
        else if (std::strcmp(key, "refine-reasons") == 0) {
            set_value(Target::RefineReasons, config, parse_bool_thread(value));
        }
        else if (std::strcmp(key, "refine-introduce") == 0) {
            set_value(Target::RefineIntroduce, config, parse_bool_thread(value));
        }
        else if (std::strcmp(key, "propagate-chain") == 0) {
            set_value(Target::PropagateChain, config, parse_bool_thread(value));
        }
        else if (std::strcmp(key, "split-all") == 0) {
            set_value(Target::SplitAll, config, parse_bool_thread(value));
        }
    }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_register_options(clingcon_theory_t *theory, clingo_options_t* options) {
    CLINGCON_TRY {
        char const *group = "CSP Options";
        auto &config = theory->propagator.config();
        Clingo::ClingoOptions opts{options, theory->parsers};

        // translation
        opts.add_flag(
            group, "shift-constraints",
            format("Shift constraints into head of integrity constraints [", flag_str(theory->shift_constraints), "]").c_str(),
            theory->shift_constraints);
        opts.add_flag(
            group, "sort-constraints",
            format("Sort constraint elements [", flag_str(config.sort_constraints), "]").c_str(),
            config.sort_constraints);
        opts.add(
            group, "translate-clauses",
            format("Restrict translation to <n> clauses per constraint [", config.clause_limit, "]").c_str(),
            parser_num<uint32_t>(config.clause_limit), false, "<n>");
        opts.add_flag(
            group, "literals-only",
            format("Only create literals during translation but no clauses [", flag_str(config.literals_only), "]").c_str(),
            config.literals_only);
        opts.add(
            group, "translate-pb",
            format("Restrict translation to <n> literals per pb constraint [", config.weight_constraint_limit, "]").c_str(),
            parser_num<uint32_t>(config.weight_constraint_limit), false, "<n>");
        opts.add(
            group, "translate-distinct",
            format("Restrict translation of distinct constraints <n> pb constraints [", config.distinct_limit, "]").c_str(),
            parser_num<uint32_t>(config.distinct_limit), false, "<n>");
        opts.add_flag(
            group, "translate-opt",
            format("Translate minimize constraint into clasp's minimize constraint [", flag_str(config.translate_minimize), "]\n").c_str(),
            config.translate_minimize);

        // propagation
        opts.add(
            group, "refine-reasons",
            format(
                "Refine reasons during propagation [", flag_str(config.default_solver_config.refine_reasons), "]\n"
                "      <arg>: {{yes|no}}[,<i>]\n"
                "      <i>: Only enable for thread <i>").c_str(),
            parser_bool_thread(*theory, Target::RefineReasons), true);
        opts.add(
            group, "refine-introduce",
            format(
                "Introduce order literals when generating reasons [", flag_str(config.default_solver_config.refine_introduce), "]\n"
                "      <arg>: {{yes|no}}[,<i>]\n"
                "      <i>: Only enable for thread <i>").c_str(),
            parser_bool_thread(*theory, Target::RefineIntroduce), true);
        opts.add(
            group, "propagate-chain",
            format(
                "Use closest order literal as reason [", flag_str(config.default_solver_config.propagate_chain), "]\n"
                "      <arg>: {{yes|no}}[,<i>]\n"
                "      <i>: Only enable for thread <i>").c_str(),
            parser_bool_thread(*theory, Target::PropagateChain), true);
        opts.add(
            group, "split-all",
            format(
                "Split all domains on total assignment [", flag_str(config.default_solver_config.split_all), "]\n"
                "      <arg>: {{yes|no}}[,<i>]\n"
                "      <i>: Only enable for thread <i>").c_str(),
            parser_bool_thread(*theory, Target::SplitAll), true);

        // hidden/debug
        opts.add(
            group, "min-int,@2",
            format("Set minimum integer [", config.min_int, "]").c_str(),
            parser_num<val_t, MIN_VAL, MAX_VAL>(config.min_int), false, "<i>");
        opts.add(
            group, "max-int,@2",
            format("Set maximum integer [", config.max_int, "]").c_str(),
            parser_num<val_t, MIN_VAL, MAX_VAL>(config.max_int), false, "<i>");
        opts.add_flag(
            group, "check-solution,@2",
            format("Verify solutions [", flag_str(config.check_solution), "]").c_str(),
            config.check_solution);
        opts.add_flag(
            group, "check-state,@2",
            format("Check state invariants [", flag_str(config.check_state), "]").c_str(),
            config.check_state);
    }
    CLINGCON_CATCH;
}

extern "C" bool clingcon_validate_options(clingcon_theory_t *theory) {
    CLINGCON_TRY {
        auto &config = theory->propagator.config();

        for (auto has_value : {false, true}) {
            for (auto [target_thread, value] : theory->deferred) {
                auto [target, thread] = target_thread;
                if (has_value == thread.has_value()) {
                    set_value(target, config, {value, thread});
                }
            }
        }
        theory->deferred.clear();

        if (config.min_int > config.max_int) {
            throw std::runtime_error("min-int must be smaller than or equal to max-int");
        }
    }
    CLINGCON_CATCH;
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
