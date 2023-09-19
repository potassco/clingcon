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
#include "clingcon/parsing.hh"
#include "clingcon/propagator.hh"

#include <clingo.hh>
#include <cstring>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>

#define CLINGCON_TRY try // NOLINT
#define CLINGCON_CATCH                                                                                                 \
    catch (...) {                                                                                                      \
        Clingo::Detail::handle_cxx_error();                                                                            \
        return false;                                                                                                  \
    }                                                                                                                  \
    return true // NOLINT

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

auto init(clingo_propagate_init_t *c_init, void *data) -> bool {
    CLINGCON_TRY {
        Clingo::PropagateInit init{c_init};
        static_cast<Propagator *>(data)->init(init);
    }
    CLINGCON_CATCH;
}

auto propagate(clingo_propagate_control_t *c_ctl, const clingo_literal_t *changes, size_t size, void *data) -> bool {
    CLINGCON_TRY {
        Clingo::PropagateControl ctl{c_ctl};
        static_cast<Propagator *>(data)->propagate(ctl, {changes, size});
    }
    CLINGCON_CATCH;
}

void undo(clingo_propagate_control_t const *c_ctl, clingo_literal_t const *changes, size_t size, void *data) {
    Clingo::PropagateControl ctl(const_cast<clingo_propagate_control_t *>(c_ctl)); // NOLINT
    static_cast<Propagator *>(data)->undo(ctl, {changes, size});
}

auto check(clingo_propagate_control_t *c_ctl, void *data) -> bool {
    CLINGCON_TRY {
        Clingo::PropagateControl ctl{c_ctl};
        static_cast<Propagator *>(data)->check(ctl);
    }
    CLINGCON_CATCH;
}

auto decide(clingo_id_t thread_id, clingo_assignment_t const *c_ass, clingo_literal_t fallback, void *data,
            clingo_literal_t *result) -> bool {
    CLINGCON_TRY {
        Clingo::Assignment ass{c_ass};
        *result = static_cast<Propagator *>(data)->decide(thread_id, ass, fallback);
    }
    CLINGCON_CATCH;
}

auto flag_str(bool value) -> char const * { return value ? "yes" : "no"; }

auto heuristic_str(Heuristic heu) -> char const * {
    switch (heu) {
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

template <typename... Args> [[nodiscard]] auto format(Args &&...args) -> std::string {
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args));
    return oss.str();
}

template <class T> [[nodiscard]] auto strtonum(char const *ib, char const *ie) -> T {
    if (!ie) {
        ie = ib + std::strlen(ib); // NOLINT
    }
    std::istringstream iss{std::string{ib, ie}};
    iss.imbue(std::locale::classic());
    iss.exceptions(std::iostream::failbit);
    iss.unsetf(std::ios_base::skipws);
    T val;
    iss >> val;
    if (!iss.eof()) {
        throw std::runtime_error("number expected");
    }
    return val;
}

template <class T>
[[nodiscard]] auto parse_range_num(char const *begin, char const *end = nullptr,
                                   T min = std::numeric_limits<T>::lowest(), T max = std::numeric_limits<T>::max())
    -> T {
    assert(min <= max);
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

template <class T>
[[nodiscard]] auto parse_num(char const *begin, T min = std::numeric_limits<T>::lowest(),
                             T max = std::numeric_limits<T>::max()) -> T {
    return parse_range_num(begin, nullptr, min, max);
}

template <class T>
[[nodiscard]] auto parser_num(T &dest, T min = std::numeric_limits<T>::lowest(), T max = std::numeric_limits<T>::max())
    -> std::function<bool(const char *)> {
    return [&dest, min, max](char const *value) {
        dest = parse_num<T>(value, min, max);
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
    } else {
        set_value(target, config.default_solver_config, val);
        for (auto &sconf : config.solver_configs) {
            set_value(target, sconf, val);
        }
    }
}

[[nodiscard]] auto parse_bool(char const *begin, char const *end = nullptr) -> bool {
    size_t len = end != nullptr ? end - begin : std::strlen(begin);
    if (std::strncmp(begin, "true", len) == 0 || std::strncmp(begin, "yes", len) == 0 ||
        std::strncmp(begin, "1", len) == 0) {
        return true;
    }
    if (std::strncmp(begin, "false", len) == 0 || std::strncmp(begin, "no", len) == 0 ||
        std::strncmp(begin, "0", len) == 0) {
        return false;
    }
    throw std::invalid_argument("invalid argument");
}

[[nodiscard]] auto find_str(char const *s, char c) -> char const * {
    if (char const *t = std::strchr(s, c); t != nullptr) {
        return t;
    }
    return s + std::strlen(s); // NOLINT
}

[[nodiscard]] auto parse_bool_thread(char const *value) -> std::pair<val_t, std::optional<uint32_t>> {
    std::optional<uint32_t> thread = std::nullopt;
    char const *comma = find_str(value, ',');
    if (*comma != '\0') {
        thread = parse_num<uint32_t>(comma + 1, 0, MAX_THREADS - 1); // NOLINT
    }

    return {parse_bool(value, comma) ? 1 : 0, thread};
}

[[nodiscard]] auto parse_sign_value(char const *value) -> std::pair<val_t, std::optional<uint32_t>> {
    std::optional<uint32_t> thread = std::nullopt;
    char const *comma = find_str(value, ',');
    if (*comma != '\0') {
        thread = parse_num<uint32_t>(comma + 1, 0, MAX_THREADS - 1); // NOLINT
    }

    if (std::strncmp(value, "+", comma - value) == 0) {
        return {std::numeric_limits<val_t>::max(), thread};
    }
    if (std::strncmp(value, "-", comma - value) == 0) {
        return {std::numeric_limits<val_t>::min(), thread};
    }
    return {parse_range_num<val_t>(value, comma), thread};
}

[[nodiscard]] auto parse_translate_clause(char const *value) -> std::pair<uint32_t, std::optional<uint64_t>> {
    std::optional<val_t> total = std::nullopt;
    char const *comma = find_str(value, ',');
    if (*comma != '\0') {
        total = parse_num<val_t>(comma + 1); // NOLINT
    }
    return {parse_range_num<uint32_t>(value, comma), total};
}

[[nodiscard]] auto parse_heuristic(char const *value) -> std::pair<val_t, std::optional<uint32_t>> {
    std::optional<uint32_t> thread = std::nullopt;
    char const *comma = find_str(value, ',');
    if (*comma != '\0') {
        thread = parse_num<uint32_t>(comma + 1, 0, MAX_THREADS - 1); // NOLINT
    }

    if (std::strncmp(value, "none", comma - value) == 0) {
        return {static_cast<val_t>(Heuristic::None), thread};
    }
    if (std::strncmp(value, "max-chain", comma - value) == 0) {
        return {static_cast<val_t>(Heuristic::MaxChain), thread};
    }
    throw std::invalid_argument("invalid argument");
}

[[nodiscard]] auto parser_bool_thread(clingcon_theory &theory, Target target) -> std::function<bool(const char *)> {
    return [&theory, target](char const *value) {
        auto [val, thread] = parse_bool_thread(value);
        return theory.deferred.emplace(std::pair(target, thread), val).second;
    };
}

[[nodiscard]] auto parser_sign_value(clingcon_theory &theory, Target target) -> std::function<bool(const char *)> {
    return [&theory, target](char const *value) {
        auto [val, thread] = parse_sign_value(value);
        return theory.deferred.emplace(std::pair(target, thread), val).second;
    };
}

[[nodiscard]] auto parser_heuristic(clingcon_theory &theory) -> std::function<bool(const char *)> {
    return [&theory](char const *value) {
        auto [val, thread] = parse_heuristic(value);
        return theory.deferred.emplace(std::pair(Target::Heuristic, thread), val).second;
    };
}

template <class T, class U>
[[nodiscard]] auto parser_translate_clause(T &translate_clauses, U &translate_clauses_total)
    -> std::function<bool(const char *)> {
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

extern "C" auto clingcon_create(clingcon_theory_t **theory) -> bool {
    CLINGCON_TRY {
        *theory = new clingcon_theory(); // NOLINT
    }
    CLINGCON_CATCH;
}

extern "C" void clingcon_version(int *major, int *minor, int *patch) {
    if (major != nullptr) {
        *major = CLINGCON_VERSION_MAJOR;
    }
    if (minor != nullptr) {
        *minor = CLINGCON_VERSION_MINOR;
    }
    if (patch != nullptr) {
        *patch = CLINGCON_VERSION_REVISION;
    }
}

extern "C" auto clingcon_register(clingcon_theory_t *theory, clingo_control_t *control) -> bool {
    // Note: The decide function is passed here for performance reasons.
    auto &config = theory->propagator.config();
    bool has_heuristic = config.default_solver_config.heuristic != Heuristic::None;
    for (auto &sconfig : config.solver_configs) {
        if (has_heuristic) {
            break;
        }
        has_heuristic = sconfig.heuristic != Heuristic::None;
    }

    static clingo_propagator_t propagator = {init, propagate, undo, check, has_heuristic ? decide : nullptr};
    return clingo_control_add(control, "base", nullptr, 0, Clingcon::THEORY) &&
           clingo_control_register_propagator(control, &propagator, &theory->propagator, false);
}

extern "C" auto clingcon_rewrite_ast(clingcon_theory_t *theory, clingo_ast_t *ast, clingcon_ast_callback_t add,
                                     void *data) -> bool {
    CLINGCON_TRY {
        clingo_ast_acquire(ast);
        Clingo::AST::Node ast_cpp{ast};
        transform(
            ast_cpp, [add, data](Clingo::AST::Node &&ast_trans) { handle_error(add(ast_trans.to_c(), data)); },
            theory->shift_constraints);
    }
    CLINGCON_CATCH;
}

extern "C" auto clingcon_prepare(clingcon_theory_t *theory, clingo_control_t *control) -> bool {
    static_cast<void>(theory);
    CLINGCON_TRY {
        Clingo::Control ctl{control, false};
        auto cnf = ctl.configuration()["solve"]["models"];
        if (cnf.value() == "-1") {
            for (auto atom : ctl.theory_atoms()) {
                auto term = atom.term();
                if ((match(term, "minimize", 0) || match(term, "maximize", 0)) && !atom.elements().empty()) {
                    cnf = "0";
                    break;
                }
            }
        }
    }
    CLINGCON_CATCH;
}

extern "C" auto clingcon_destroy(clingcon_theory_t *theory) -> bool {
    delete theory; // NOLINT
    return true;
}

extern "C" auto clingcon_configure(clingcon_theory_t *theory, char const *key, char const *value) -> bool {
    CLINGCON_TRY {
        auto &config = theory->propagator.config();
        // translation
        if (std::strcmp(key, "shift-constraints") == 0) {
            theory->shift_constraints = parse_bool(value);
        } else if (std::strcmp(key, "sort-constraints") == 0) {
            config.sort_constraints = parse_bool(value);
        } else if (std::strcmp(key, "translate-clauses") == 0) {
            auto [clauses, clauses_total] = parse_translate_clause(value);
            config.clause_limit = clauses;
            if (clauses_total) {
                config.clause_limit_total = *clauses_total;
            }
        } else if (std::strcmp(key, "literals-only") == 0) {
            config.literals_only = parse_bool(value);
        } else if (std::strcmp(key, "translate-pb") == 0) {
            config.weight_constraint_ratio = parse_num<double>(value);
        } else if (std::strcmp(key, "translate-distinct") == 0) {
            config.distinct_limit = parse_num<uint32_t>(value);
        } else if (std::strcmp(key, "translate-opt") == 0) {
            config.translate_minimize = parse_num<uint32_t>(value);
        } else if (std::strcmp(key, "add-order-clauses") == 0) {
            config.add_order_clauses = parse_bool(value);
        }
        // hidden/debug
        else if (std::strcmp(key, "min-int") == 0) {
            config.min_int = parse_num<val_t>(value, MIN_VAL, MAX_VAL);
        } else if (std::strcmp(key, "max-int") == 0) {
            config.max_int = parse_num<val_t>(value, MIN_VAL, MAX_VAL);
        } else if (std::strcmp(key, "check-solution") == 0) {
            config.check_solution = parse_bool(value);
        } else if (std::strcmp(key, "check-state") == 0) {
            config.check_state = parse_bool(value);
        }
        // propagation
        else if (std::strcmp(key, "order-heuristic") == 0) {
            set_value(Target::Heuristic, config, parse_heuristic(value));
        } else if (std::strcmp(key, "sign-value") == 0) {
            set_value(Target::SignValue, config, parse_sign_value(value));
        } else if (std::strcmp(key, "refine-reasons") == 0) {
            set_value(Target::RefineReasons, config, parse_bool_thread(value));
        } else if (std::strcmp(key, "refine-introduce") == 0) {
            set_value(Target::RefineIntroduce, config, parse_bool_thread(value));
        } else if (std::strcmp(key, "propagate-chain") == 0) {
            set_value(Target::PropagateChain, config, parse_bool_thread(value));
        } else if (std::strcmp(key, "split-all") == 0) {
            set_value(Target::SplitAll, config, parse_bool_thread(value));
        }
    }
    CLINGCON_CATCH;
}

extern "C" auto clingcon_register_options(clingcon_theory_t *theory, clingo_options_t *options) -> bool {
    CLINGCON_TRY {
        char const *group = "CSP Options";
        auto &config = theory->propagator.config();
        Clingo::ClingoOptions opts{options, theory->parsers};

        // translation
        opts.add_flag(
            group, "shift-constraints",
            format("Shift constraints into head of integrity constraints [", flag_str(theory->shift_constraints), "]")
                .c_str(),
            theory->shift_constraints);
        opts.add_flag(group, "sort-constraints",
                      format("Sort constraint elements [", flag_str(config.sort_constraints), "]").c_str(),
                      config.sort_constraints);
        opts.add(group, "translate-clauses",
                 format("Restrict translation to clauses [", config.clause_limit, ",", config.clause_limit_total, "]\n",
                        "      <n>: maximum clauses per constraint\n"
                        "      <m>: maximum clauses total")
                     .c_str(),
                 parser_translate_clause(config.clause_limit, config.clause_limit_total), false, "<n>[,<m>]");
        opts.add_flag(
            group, "literals-only",
            format("Only create literals during translation but no clauses [", flag_str(config.literals_only), "]")
                .c_str(),
            config.literals_only);
        opts.add(group, "translate-pb",
                 format("Translate to weight constraints if ratio of variables and literals is less equal <r> [",
                        config.weight_constraint_ratio, "]")
                     .c_str(),
                 parser_num(config.weight_constraint_ratio), false, "<r>");
        opts.add(
            group, "translate-distinct",
            format("Restrict translation of distinct constraints to <n> pb constraints [", config.distinct_limit, "]")
                .c_str(),
            parser_num<uint32_t>(config.distinct_limit), false, "<n>");
        opts.add(group, "translate-opt",
                 format("Configure translation of minimize constraint [", config.translate_minimize,
                        "]\n"
                        "      <n>: translate if required literals less than <n>\n"
                        "        0  : never translate\n"
                        "        max: always translate")
                     .c_str(),
                 parser_num<uint32_t>(config.translate_minimize), false, "<n>");
        opts.add_flag(
            group, "add-order-clauses",
            format("Add binary clauses for order literals after translation [", flag_str(config.add_order_clauses), "]")
                .c_str(),
            config.add_order_clauses);

        // propagation
        opts.add(group, "order-heuristic",
                 format("Make the decision heuristic aware of order literls [",
                        heuristic_str(config.default_solver_config.heuristic),
                        "]\n"
                        "      <arg>: {none,max-chain}[,<i>]\n"
                        "        none     : use clasp's heuristic\n"
                        "        max-chain: assign chains of literals\n"
                        "      <i>  : Only enable for thread <i>")
                     .c_str(),
                 parser_heuristic(*theory), true);
        opts.add(group, "sign-value",
                 format("Configure the sign of order literals [", config.default_solver_config.sign_value,
                        "]\n"
                        "      <arg>: {<n>|+|-}[,<i>]\n"
                        "        <n>: negative iff its value is greater or equal to <n>\n"
                        "        +  : always positive\n"
                        "        -  : always negative\n"
                        "      <i>  : Only enable for thread <i>")
                     .c_str(),
                 parser_sign_value(*theory, Target::SignValue), true);
        opts.add(group, "refine-reasons",
                 format("Refine reasons during propagation [", flag_str(config.default_solver_config.refine_reasons),
                        "]\n"
                        "      <arg>: {yes|no}[,<i>]\n"
                        "      <i>  : Only enable for thread <i>")
                     .c_str(),
                 parser_bool_thread(*theory, Target::RefineReasons), true);
        opts.add(group, "refine-introduce",
                 format("Introduce order literals when generating reasons [",
                        flag_str(config.default_solver_config.refine_introduce),
                        "]\n"
                        "      <arg>: {yes|no}[,<i>]\n"
                        "      <i>  : Only enable for thread <i>")
                     .c_str(),
                 parser_bool_thread(*theory, Target::RefineIntroduce), true);
        opts.add(group, "propagate-chain",
                 format("Use closest order literal as reason [", flag_str(config.default_solver_config.propagate_chain),
                        "]\n"
                        "      <arg>: {yes|no}[,<i>]\n"
                        "      <i>  : Only enable for thread <i>")
                     .c_str(),
                 parser_bool_thread(*theory, Target::PropagateChain), true);
        opts.add(group, "split-all",
                 format("Split all domains on total assignment [", flag_str(config.default_solver_config.split_all),
                        "]\n"
                        "      <arg>: {yes|no}[,<i>]\n"
                        "      <i>  : Only enable for thread <i>")
                     .c_str(),
                 parser_bool_thread(*theory, Target::SplitAll), true);

        // hidden/debug
        opts.add(group, "min-int,@2", format("Set minimum integer [", config.min_int, "]").c_str(),
                 parser_num<val_t>(config.min_int, MIN_VAL, MAX_VAL), false, "<i>");
        opts.add(group, "max-int,@2", format("Set maximum integer [", config.max_int, "]").c_str(),
                 parser_num<val_t>(config.max_int, MIN_VAL, MAX_VAL), false, "<i>");
        opts.add_flag(group, "check-solution,@2",
                      format("Verify solutions [", flag_str(config.check_solution), "]").c_str(),
                      config.check_solution);
        opts.add_flag(group, "check-state,@2",
                      format("Check state invariants [", flag_str(config.check_state), "]").c_str(),
                      config.check_state);
    }
    CLINGCON_CATCH;
}

extern "C" auto clingcon_validate_options(clingcon_theory_t *theory) -> bool {
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

extern "C" auto clingcon_on_model(clingcon_theory_t *theory, clingo_model_t *model) -> bool {
    CLINGCON_TRY {
        Clingo::Model m{model};
        theory->propagator.on_model(m);
    }
    CLINGCON_CATCH;
}

extern "C" auto clingcon_lookup_symbol(clingcon_theory_t *theory, clingo_symbol_t symbol, size_t *index) -> bool {
    if (auto var = theory->propagator.get_index(Clingo::Symbol{symbol}); var.has_value()) {
        *index = *var + 1;
        return true;
    }
    return false;
}

extern "C" auto clingcon_get_symbol(clingcon_theory_t *theory, size_t index) -> clingo_symbol_t {
    auto sym = theory->propagator.get_symbol(index - 1);
    assert(sym.has_value());
    return sym->to_c();
}

extern "C" void clingcon_assignment_begin(clingcon_theory_t *theory, uint32_t thread_id, size_t *index) {
    static_cast<void>(theory);
    static_cast<void>(thread_id);
    *index = 0;
}

extern "C" auto clingcon_assignment_next(clingcon_theory_t *theory, uint32_t thread_id, size_t *index) -> bool {
    static_cast<void>(thread_id);
    auto const &map = theory->propagator.var_map();
    auto it = map.lower_bound(*index);
    if (it != map.end()) {
        *index = *index + 1;
        return true;
    }
    return false;
}

extern "C" auto clingcon_assignment_has_value(clingcon_theory_t *theory, uint32_t thread_id, size_t index) -> bool {
    static_cast<void>(thread_id);
    return theory->propagator.get_symbol(index - 1).has_value();
}

extern "C" void clingcon_assignment_get_value(clingcon_theory_t *theory, uint32_t thread_id, size_t index,
                                              clingcon_value_t *value) {
    value->type = clingcon_value_type_int;                                  // NOLINT
    value->int_number = theory->propagator.get_value(index - 1, thread_id); // NOLINT
}

extern "C" auto clingcon_on_statistics(clingcon_theory_t *theory, clingo_statistics_t *step, clingo_statistics_t *accu)
    -> bool {
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
