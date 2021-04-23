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
enum class Target { Heuristic, SignValue, RefineReasons, RefineIntroduce, PropagateChain, SplitAll };

} // namespace

struct clingcon_theory {
    Propagator propagator;
    Clingo::Detail::ParserList parsers;
    std::map<std::pair<Target, std::optional<uint32_t>>, val_t> deferred;
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

bool decide(clingo_id_t thread_id, clingo_assignment_t const *c_ass, clingo_literal_t fallback, void* data, clingo_literal_t *result) {
    CLINGCON_TRY {
        Clingo::Assignment ass{c_ass};
        *result = static_cast<Propagator*>(data)->decide(thread_id, ass, fallback);
    }
    CLINGCON_CATCH;
}

char const *flag_str(bool value) {
    return value ? "yes" : "no";
}

char const *heuristic_str(Heuristic heu) {
    switch(heu) {
        case Heuristic::None: {
            return "none";
        }
        case Heuristic::MaxChain: {
            return "max-chain";
            break;
        }
    };
    return "";
}

template <typename... Args>
[[nodiscard]] std::string format(Args &&... args) {
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args));
    return oss.str();
}

template<class T>
[[nodiscard]] T strtonum(char const *begin, char const *end) {
    if (!end) {
        end = begin + std::strlen(begin); // NOLINT
    }
    T ret = 0;
    bool sign = false;
    auto const *it = begin;
    if constexpr (std::is_signed_v<T>) {
        if (*it == '-') {
            sign = true;
            ++it; // NOLINT
        }
    }
    else {
        static_cast<void>(sign);
    }
    if (it == end) {
        throw std::invalid_argument("integer expected");
    }
    for (; it != end; ++it) { // NOLINT
        if ('0' <= *it && *it <= '9') {
            ret = safe_add<T>(safe_mul<T>(ret, 10), *it - '0'); // NOLINT
        }
        else {
            throw std::invalid_argument("integer expected");
        }
    }
    if constexpr (std::is_signed_v<T>) {
        return sign ? safe_inv<T>(ret) : ret;
    }
    else {
        return ret;
    }
}

template<class T, T min=std::numeric_limits<T>::min(), T max=std::numeric_limits<T>::max()>
[[nodiscard]] T parse_num(char const *begin, char const *end = nullptr) {
    static_assert(min <= max);
    if (strncmp(begin, "min", end - begin) == 0) {
        return min;
    }
    if (strncmp(begin, "max", end - begin) == 0) {
        return max;
    }
    auto res = strtonum<T>(begin, end);
    if (min <= res && res <= max) {
        return res;
    }
    throw std::invalid_argument("invalid argument");
}

template<class T, T min=std::numeric_limits<T>::min(), T max=std::numeric_limits<T>::max()>
[[nodiscard]] std::function<bool (const char *)> parser_num(T &dest) {
    return [&dest](char const *value) {
        dest = parse_num<T>(value);
        return true;
    };
}

void set_value(Target target, SolverConfig &config, val_t value) {
    switch (target) {
        case Target::SignValue: {
            config.sign_value = value;
            break;
        }
        case Target::Heuristic: {
            config.heuristic = static_cast<Heuristic>(value);
            break;
        }
        case Target::RefineReasons: {
            config.refine_reasons = value != 0;
            break;
        }
        case Target::RefineIntroduce: {
            config.refine_introduce = value != 0;
            break;
        }
        case Target::PropagateChain: {
            config.propagate_chain = value != 0;
            break;
        }
        case Target::SplitAll: {
            config.split_all = value != 0;
            break;
        }
    }
}

void set_value(Target target, Config &config, std::pair<val_t, std::optional<uint32_t>> const &value) {
    auto const &[val, thread] = value;
    if (thread.has_value()) {
        set_value(target, config.solver_config(*thread), val);
    }
    else {
        set_value(target, config.default_solver_config, val);
        for (auto &sconf : config.solver_configs) {
            set_value(target, sconf, val);
        }
    }
}

[[nodiscard]] bool parse_bool(char const *begin, char const *end = nullptr) {
    size_t len = end != nullptr ? end - begin : std::strlen(begin);
    if (std::strncmp(begin, "true", len) == 0 || std::strncmp(begin, "yes", len) == 0 || std::strncmp(begin, "1", len) == 0) {
        return true;
    }
    if (std::strncmp(begin, "false", len) == 0 || std::strncmp(begin, "no", len) == 0 || std::strncmp(begin, "0", len) == 0) {
        return false;
    }
    throw std::invalid_argument("invalid argument");
}

[[nodiscard]] char const *find_str(char const *s, char c) {
    if (char const *t = std::strchr(s, c); t != nullptr) {
        return t;
    }
    return s + std::strlen(s); // NOLINT
}

[[nodiscard]] std::pair<val_t, std::optional<uint32_t>> parse_bool_thread(char const *value) {
    std::optional<uint32_t> thread = std::nullopt;
    char const *comma = find_str(value, ',');
    if (*comma != '\0') {
        thread = parse_num<uint32_t, 0, MAX_THREADS - 1>(comma + 1); // NOLINT
    }

    return {parse_bool(value, comma) ? 1 : 0, thread};
}

[[nodiscard]] std::pair<val_t, std::optional<uint32_t>> parse_sign_value(char const *value) {
    std::optional<uint32_t> thread = std::nullopt;
    char const *comma = find_str(value, ',');
    if (*comma != '\0') {
        thread = parse_num<uint32_t, 0, MAX_THREADS - 1>(comma + 1); // NOLINT
    }

    if (std::strncmp(value, "+", comma - value) == 0) {
        return {std::numeric_limits<val_t>::max(), thread};
    }
    if (std::strncmp(value, "-", comma - value) == 0) {
        return {std::numeric_limits<val_t>::min(), thread};
    }
    return {parse_num<val_t>(value, comma), thread};
}

[[nodiscard]] std::pair<uint32_t, std::optional<uint64_t>> parse_translate_clause(char const *value) {
    std::optional<val_t> total = std::nullopt;
    char const *comma = find_str(value, ',');
    if (*comma != '\0') {
        total = parse_num<val_t>(comma + 1); // NOLINT
    }
    return {parse_num<uint32_t>(value, comma), total};
}

[[nodiscard]] std::pair<val_t, std::optional<uint32_t>> parse_heuristic(char const *value) {
    std::optional<uint32_t> thread = std::nullopt;
    char const *comma = find_str(value, ',');
    if (*comma != '\0') {
        thread = parse_num<uint32_t, 0, MAX_THREADS - 1>(comma + 1); // NOLINT
    }

    if (std::strncmp(value, "none", comma - value) == 0) {
        return {static_cast<val_t>(Heuristic::None), thread};
    }
    if (std::strncmp(value, "max-chain", comma - value) == 0) {
        return {static_cast<val_t>(Heuristic::MaxChain), thread};
    }
    throw std::invalid_argument("invalid argument");
}

[[nodiscard]] std::function<bool (const char *)> parser_bool_thread(clingcon_theory &theory, Target target) {
    return [&theory, target](char const *value) {
        auto [val, thread] = parse_bool_thread(value);
        return theory.deferred.emplace(std::pair(target, thread), val).second;
    };
}

[[nodiscard]] std::function<bool (const char *)> parser_sign_value(clingcon_theory &theory, Target target) {
    return [&theory, target](char const *value) {
        auto [val, thread] = parse_sign_value(value);
        return theory.deferred.emplace(std::pair(target, thread), val).second;
    };
}

[[nodiscard]] std::function<bool (const char *)> parser_heuristic(clingcon_theory &theory) {
    return [&theory](char const *value) {
        auto [val, thread] = parse_heuristic(value);
        return theory.deferred.emplace(std::pair(Target::Heuristic, thread), val).second;
    };
}

template<class T, class U>
[[nodiscard]] std::function<bool (const char *)> parser_translate_clause(T &translate_clauses, U &translate_clauses_total) {
    return [&translate_clauses, &translate_clauses_total](char const *value) {
        auto [clauses, clauses_total] = parse_translate_clause(value);
        translate_clauses = clauses;
        if (clauses_total) {
            translate_clauses_total = *clauses_total;
        }
        return true;
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
    // Note: The decide function is passed here for performance reasons.
    auto &config = theory->propagator.config();
    bool has_heuristic = config.default_solver_config.heuristic != Heuristic::None;
    for (auto &sconfig : config.solver_configs) {
        if (has_heuristic) { break; }
        has_heuristic = sconfig.heuristic != Heuristic::None;
    }

    static clingo_propagator_t propagator = { init, propagate, undo, check, has_heuristic ? decide : nullptr };
    return
        clingo_control_add(control, "base", nullptr, 0, Clingcon::THEORY) &&
        clingo_control_register_propagator(control, &propagator, &theory->propagator, false);
}

extern "C" bool clingcon_rewrite_ast(clingcon_theory_t *theory, clingo_ast_t *ast, clingcon_ast_callback_t add, void *data) {
    CLINGCON_TRY {
        clingo_ast_acquire(ast);
        Clingo::AST::Node ast_cpp{ast};
        transform(ast_cpp, [add, data](Clingo::AST::Node &&ast_trans){
            handle_error(add(ast_trans.to_c(), data));
        }, theory->shift_constraints);
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
            auto [clauses, clauses_total] = parse_translate_clause(value);
            config.clause_limit = clauses;
            if (clauses_total) {
                config.clause_limit_total = *clauses_total;
            }
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
            config.translate_minimize = parse_num<uint32_t>(value);
        }
        else if (std::strcmp(key, "add-order-clauses") == 0) {
            config.add_order_clauses = parse_bool(value);
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
        else if (std::strcmp(key, "order-heuristic") == 0) {
            set_value(Target::Heuristic, config, parse_heuristic(value));
        }
        else if (std::strcmp(key, "sign-value") == 0) {
            set_value(Target::SignValue, config, parse_sign_value(value));
        }
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
            format(
                "Restrict translation to clauses [", config.clause_limit, ",", config.clause_limit_total,"]\n",
                "      <n>: maximum clauses per constraint\n"
                "      <m>: maximum clauses total").c_str(),
            parser_translate_clause(config.clause_limit, config.clause_limit_total), false, "<n>[,<m>]");
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
        opts.add(
            group, "translate-opt",
            format(
                "Configure translation of minimize constraint [", config.translate_minimize, "]\n"
                "      <n>: translate if required literals less equal to <n>\n"
                "        0  : never translate\n"
                "        max: always translate").c_str(),
            parser_num<uint32_t>(config.translate_minimize), false, "<n>");
        opts.add_flag(
            group, "add-order-clauses",
            format("Add binary clauses for order literals after translation [", flag_str(config.add_order_clauses), "]").c_str(),
            config.add_order_clauses);

        // propagation
        opts.add(
            group, "order-heuristic",
            format(
                "Make the decision heuristic aware of order literls [", heuristic_str(config.default_solver_config.heuristic), "]\n"
                "      <arg>: {none,max-chain}[,<i>]\n"
                "        none     : use clasp's heuristic\n"
                "        max-chain: assign chains of literals\n"
                "      <i>  : Only enable for thread <i>").c_str(),
            parser_heuristic(*theory), true);
        opts.add(
            group, "sign-value",
            format(
                "Configure the sign of order literals [", config.default_solver_config.sign_value, "]\n"
                "      <arg>: {<n>|+|-}[,<i>]\n"
                "        <n>: negative iff its value is greater or equal to <n>\n"
                "        +  : always positive\n"
                "        -  : always negative\n"
                "      <i>  : Only enable for thread <i>").c_str(),
            parser_sign_value(*theory, Target::SignValue), true);
        opts.add(
            group, "refine-reasons",
            format(
                "Refine reasons during propagation [", flag_str(config.default_solver_config.refine_reasons), "]\n"
                "      <arg>: {yes|no}[,<i>]\n"
                "      <i>  : Only enable for thread <i>").c_str(),
            parser_bool_thread(*theory, Target::RefineReasons), true);
        opts.add(
            group, "refine-introduce",
            format(
                "Introduce order literals when generating reasons [", flag_str(config.default_solver_config.refine_introduce), "]\n"
                "      <arg>: {yes|no}[,<i>]\n"
                "      <i>  : Only enable for thread <i>").c_str(),
            parser_bool_thread(*theory, Target::RefineIntroduce), true);
        opts.add(
            group, "propagate-chain",
            format(
                "Use closest order literal as reason [", flag_str(config.default_solver_config.propagate_chain), "]\n"
                "      <arg>: {yes|no}[,<i>]\n"
                "      <i>  : Only enable for thread <i>").c_str(),
            parser_bool_thread(*theory, Target::PropagateChain), true);
        opts.add(
            group, "split-all",
            format(
                "Split all domains on total assignment [", flag_str(config.default_solver_config.split_all), "]\n"
                "      <arg>: {yes|no}[,<i>]\n"
                "      <i>  : Only enable for thread <i>").c_str(),
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
