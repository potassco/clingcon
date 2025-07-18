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

#include <clingo/app.hh>
#include <clingo/control.h>
#include <clingo/control.hh>

#include <cstring>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

using Clingo::Detail::handle_error;

using namespace Clingcon;

namespace {

constexpr uint32_t MAX_THREADS = 64;
enum class Target : uint8_t { Heuristic, SignValue, RefineReasons, RefineIntroduce, PropagateChain, SplitAll };

using Deferred = std::map<std::pair<Target, std::optional<uint32_t>>, val_t>;

auto init(clingo_assignment_t const *assignment, clingo_propagate_init_t *c_init, void *data) -> bool {
    CLINGO_TRY {
        Clingo::PropagateInit init{c_init};
        static_cast<Propagator *>(data)->init(Clingo::Assignment{assignment}, init);
    }
    CLINGO_CATCH;
}

auto propagate(clingo_assignment_t const *assignment, clingo_propagate_control_t *c_ctl,
               clingo_literal_t const *changes, size_t size, void *data) -> bool {
    CLINGO_TRY {
        Clingo::PropagateControl ctl{c_ctl};
        static_cast<Propagator *>(data)->propagate(Clingo::Assignment{assignment}, ctl, {changes, size});
    }
    CLINGO_CATCH;
}

void undo(clingo_assignment_t const *assignment, clingo_literal_t const *changes, size_t size, void *data) {
    try {
        static_cast<Propagator *>(data)->undo(Clingo::Assignment{assignment}, {changes, size});
    } catch (std::exception const &e) {
        printf("panic: %s\n", e.what());
    }
}

auto check(clingo_assignment_t const *assignment, clingo_propagate_control_t *control, void *data) -> bool {
    CLINGO_TRY {
        static_cast<Propagator *>(data)->check(Clingo::Assignment{assignment}, Clingo::PropagateControl{control});
    }
    CLINGO_CATCH;
}

auto decide(clingo_assignment_t const *assignment, clingo_literal_t fallback, void *data,
            clingo_literal_t *result) -> bool {
    CLINGO_TRY {
        *result = static_cast<Propagator *>(data)->decide(Clingo::Assignment{assignment}, fallback);
    }
    CLINGO_CATCH;
}

auto flag_str(bool value) -> std::string_view {
    return value ? "yes" : "no";
}

auto heuristic_str(Heuristic heu) -> std::string_view {
    switch (heu) {
        case Heuristic::None: {
            return "none";
        }
        case Heuristic::MaxChain: {
            return "max-chain";
        }
    };
    return "";
}

template <typename... Args> [[nodiscard]] auto format(Args &&...args) -> std::string {
    std::ostringstream oss;
    (oss << ... << std::forward<Args>(args)); // NOLINT
    return oss.str();
}

template <class T> [[nodiscard]] auto strtonum(std::string_view str) -> T {
    std::istringstream iss{std::string{str.begin(), str.end()}};
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
[[nodiscard]] auto parse_num(std::string_view str, T min = std::numeric_limits<T>::lowest(),
                             T max = std::numeric_limits<T>::max()) -> T {
    assert(min <= max);
    if (str == "min") {
        return min;
    }
    if (str == "max") {
        return max;
    }
    auto res = strtonum<T>(str);
    if (min <= res && res <= max) {
        return res;
    }
    throw std::invalid_argument("invalid argument");
}

template <class T>
[[nodiscard]] auto parser_num(T &dest, T min = std::numeric_limits<T>::lowest(),
                              T max = std::numeric_limits<T>::max()) -> std::function<bool(std::string_view)> {
    return [&dest, min, max](std::string_view value) {
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

[[nodiscard]] auto parse_bool(std::string_view value) -> bool {
    if (value == "true" || value == "yes" || value == "1") {
        return true;
    }
    if (value == "false" || value == "no" || value == "0") {
        return false;
    }
    throw std::invalid_argument("invalid argument");
}

[[nodiscard]] auto parse_bool_thread(std::string_view value) -> std::pair<val_t, std::optional<uint32_t>> {
    std::optional<uint32_t> thread = std::nullopt;
    auto comma = value.find(',');
    if (comma != std::string_view::npos) {
        thread = parse_num<uint32_t>(value.substr(comma + 1), 0, MAX_THREADS - 1); // NOLINT
    }
    return {parse_bool(value.substr(0, comma)) ? 1 : 0, thread};
}

[[nodiscard]] auto parse_sign_value(std::string_view value) -> std::pair<val_t, std::optional<uint32_t>> {
    std::optional<uint32_t> thread = std::nullopt;
    auto comma = value.find(',');
    if (comma != std::string_view::npos) {
        thread = parse_num<uint32_t>(value.substr(comma + 1), 0, MAX_THREADS - 1); // NOLINT
    }
    auto rem = value.substr(0, comma);
    if (rem == "+") {
        return {std::numeric_limits<val_t>::max(), thread};
    }
    if (rem == "-") {
        return {std::numeric_limits<val_t>::min(), thread};
    }
    return {parse_num<val_t>(rem), thread};
}

[[nodiscard]] auto parse_translate_clause(std::string_view value) -> std::pair<uint32_t, std::optional<uint64_t>> {
    std::optional<val_t> total = std::nullopt;
    auto comma = value.find(',');
    if (comma != std::string_view::npos) {
        total = parse_num<val_t>(value.substr(comma + 1)); // NOLINT
    }
    return {parse_num<uint32_t>(value.substr(0, comma)), total};
}

[[nodiscard]] auto parse_heuristic(std::string_view value) -> std::pair<val_t, std::optional<uint32_t>> {
    std::optional<uint32_t> thread = std::nullopt;
    auto comma = value.find(',');
    if (comma != std::string_view::npos) {
        thread = parse_num<uint32_t>(value.substr(comma + 1), 0, MAX_THREADS - 1); // NOLINT
    }
    auto prefix = value.substr(0, comma);
    if (prefix == "none") {
        return {static_cast<val_t>(Heuristic::None), thread};
    }
    if (prefix == "max-chain") {
        return {static_cast<val_t>(Heuristic::MaxChain), thread};
    }
    throw std::invalid_argument("invalid argument");
}

[[nodiscard]] auto parser_bool_thread(Deferred &deferred, Target target) -> std::function<bool(std::string_view)> {
    return [&deferred, target](std::string_view value) {
        auto [val, thread] = parse_bool_thread(value);
        return deferred.emplace(std::pair(target, thread), val).second;
    };
}

[[nodiscard]] auto parser_sign_value(Deferred &deferred, Target target) -> std::function<bool(std::string_view)> {
    return [&deferred, target](std::string_view value) {
        auto [val, thread] = parse_sign_value(value);
        return deferred.emplace(std::pair(target, thread), val).second;
    };
}

[[nodiscard]] auto parser_heuristic(Deferred &deferred) -> std::function<bool(std::string_view)> {
    return [&deferred](std::string_view value) {
        auto [val, thread] = parse_heuristic(value);
        return deferred.emplace(std::pair(Target::Heuristic, thread), val).second;
    };
}

template <class T, class U>
[[nodiscard]] auto parser_translate_clause(T &translate_clauses,
                                           U &translate_clauses_total) -> std::function<bool(std::string_view)> {
    return [&translate_clauses, &translate_clauses_total](std::string_view value) {
        auto [clauses, clauses_total] = parse_translate_clause(value);
        translate_clauses = clauses;
        if (clauses_total) {
            translate_clauses_total = *clauses_total;
        }
        return true;
    };
}

struct clingcon_theory {
    static auto info([[maybe_unused]] void *self, clingo_string_t *name, int *major, int *minor, int *patch) -> bool {
        using namespace std::string_view_literals;
        CLINGO_TRY {
            if (name != nullptr) {
                constexpr auto str = "clingcon"sv;
                name->data = str.data();
                name->size = str.size();
            }
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
        CLINGO_CATCH;
    }

    static auto register_(void *self, clingo_control_t *control) -> bool {
        CLINGO_TRY {
            auto *theory = static_cast<clingcon_theory *>(self);
            // Note: The decide function is passed here for performance reasons.
            auto &config = theory->propagator.config();
            bool has_heuristic = config.default_solver_config.heuristic != Heuristic::None;
            for (auto &sconfig : config.solver_configs) {
                if (has_heuristic) {
                    break;
                }
                has_heuristic = sconfig.heuristic != Heuristic::None;
            }

            static clingo_propagator_t propagator = {
                init, nullptr, propagate, undo, check, has_heuristic ? decide : nullptr, nullptr};
            return clingo_control_parse_string(control, Clingcon::THEORY, std::strlen(Clingcon::THEORY)) &&
                   clingo_control_register_propagator(control, &propagator, &theory->propagator);
        }
        CLINGO_CATCH;
    }

    static auto rewrite_ast(void *self, clingo_ast_t *ast, clingo_theory_ast_callback_t add, void *data) -> bool {
        auto *theory = static_cast<clingcon_theory *>(self);
        CLINGO_TRY {
            transform(
                theory->lib, Clingo::AST::Node{ast, true},
                [add, data](Clingo::AST::Node const &ast) { handle_error(add(c_cast(ast), data)); },
                theory->shift_constraints);
        }
        CLINGO_CATCH;
    }

    static auto prepare([[maybe_unused]] void *self, clingo_control_t *control) -> bool {
        CLINGO_TRY {
            Clingo::Control ctl{control, true};
            auto cnf = ctl.config()["solve"]["models"];
            if (cnf.value() == "-1") {
                for (auto atom : ctl.base().theory()) {
                    auto term = atom.name();
                    if ((match(term, "minimize", 0) || match(term, "maximize", 0)) && !atom.elements().empty()) {
                        cnf = "0";
                        break;
                    }
                }
            }
        }
        CLINGO_CATCH;
    }

    static void destroy(void *self) {
        auto *theory = static_cast<clingcon_theory *>(self);
        std::unique_ptr<clingcon_theory>{theory};
    }

    static auto configure(void *self, char const *key_data, size_t key_size, char const *value_data,
                          size_t value_size) -> bool {
        CLINGO_TRY {
            auto *theory = static_cast<clingcon_theory *>(self);
            auto key = std::string_view{key_data, key_size};
            auto value = std::string_view{value_data, value_size};
            auto &config = theory->propagator.config();
            // translation
            if (key == "shift-constraints") {
                theory->shift_constraints = parse_bool(value);
            } else if (key == "sort-constraints") {
                config.sort_constraints = parse_bool(value);
            } else if (key == "translate-clauses") {
                auto [clauses, clauses_total] = parse_translate_clause(value);
                config.clause_limit = clauses;
                if (clauses_total) {
                    config.clause_limit_total = *clauses_total;
                }
            } else if (key == "literals-only") {
                config.literals_only = parse_bool(value);
            } else if (key == "translate-pb") {
                config.weight_constraint_ratio = parse_num<double>(value);
            } else if (key == "translate-distinct") {
                config.distinct_limit = parse_num<uint32_t>(value);
            } else if (key == "translate-opt") {
                config.translate_minimize = parse_num<uint32_t>(value);
            } else if (key == "add-order-clauses") {
                config.add_order_clauses = parse_bool(value);
            }
            // hidden/debug
            else if (key == "min-int") {
                config.min_int = parse_num<val_t>(value, MIN_VAL, MAX_VAL);
            } else if (key == "max-int") {
                config.max_int = parse_num<val_t>(value, MIN_VAL, MAX_VAL);
            } else if (key == "check-solution") {
                config.check_solution = parse_bool(value);
            } else if (key == "check-state") {
                config.check_state = parse_bool(value);
            }
            // propagation
            else if (key == "order-heuristic") {
                set_value(Target::Heuristic, config, parse_heuristic(value));
            } else if (key == "sign-value") {
                set_value(Target::SignValue, config, parse_sign_value(value));
            } else if (key == "refine-reasons") {
                set_value(Target::RefineReasons, config, parse_bool_thread(value));
            } else if (key == "refine-introduce") {
                set_value(Target::RefineIntroduce, config, parse_bool_thread(value));
            } else if (key == "propagate-chain") {
                set_value(Target::PropagateChain, config, parse_bool_thread(value));
            } else if (key == "split-all") {
                set_value(Target::SplitAll, config, parse_bool_thread(value));
            } else {
                throw std::invalid_argument{"unknown config key"};
            }
        }
        CLINGO_CATCH;
    }

    static auto register_options(void *self, clingo_options_t *options) -> bool {
        CLINGO_TRY {
            auto *theory = static_cast<clingcon_theory *>(self);
            std::string_view group = "CSP Options";
            auto &config = theory->propagator.config();
            Clingo::Options opts{options, theory->parsers};

            // translation
            opts.add_flag(group, "shift-constraints",
                          format("Shift constraints into head of integrity constraints [",
                                 flag_str(theory->shift_constraints), "]"),
                          theory->shift_constraints);
            opts.add_flag(group, "sort-constraints",
                          format("Sort constraint elements [", flag_str(config.sort_constraints), "]"),
                          config.sort_constraints);
            opts.add(group, "translate-clauses",
                     format("Restrict translation to clauses [", config.clause_limit, ",", config.clause_limit_total,
                            "]\n",
                            "      <n>: maximum clauses per constraint\n"
                            "      <m>: maximum clauses total"),
                     parser_translate_clause(config.clause_limit, config.clause_limit_total), false, "<n>[,<m>]");
            opts.add_flag(
                group, "literals-only",
                format("Only create literals during translation but no clauses [", flag_str(config.literals_only), "]"),
                config.literals_only);
            opts.add(group, "translate-pb",
                     format("Translate to weight constraints if ratio of variables and literals is less equal <r> [",
                            config.weight_constraint_ratio, "]"),
                     parser_num(config.weight_constraint_ratio), false, "<r>");
            opts.add(group, "translate-distinct",
                     format("Restrict translation of distinct constraints to <n> pb constraints [",
                            config.distinct_limit, "]"),
                     parser_num<uint32_t>(config.distinct_limit), false, "<n>");
            opts.add(group, "translate-opt",
                     format("Configure translation of minimize constraint [", config.translate_minimize,
                            "]\n"
                            "      <n>: translate if required literals less than <n>\n"
                            "        0  : never translate\n"
                            "        max: always translate"),
                     parser_num<uint32_t>(config.translate_minimize), false, "<n>");
            opts.add_flag(group, "add-order-clauses",
                          format("Add binary clauses for order literals after translation [",
                                 flag_str(config.add_order_clauses), "]"),
                          config.add_order_clauses);

            // propagation
            opts.add(group, "order-heuristic",
                     format("Make the decision heuristic aware of order literals [",
                            heuristic_str(config.default_solver_config.heuristic),
                            "]\n"
                            "      <arg>: {none,max-chain}[,<i>]\n"
                            "        none     : use clasp's heuristic\n"
                            "        max-chain: assign chains of literals\n"
                            "      <i>  : Only enable for thread <i>"),
                     parser_heuristic(theory->deferred), true);
            opts.add(group, "sign-value",
                     format("Configure the sign of order literals [", config.default_solver_config.sign_value,
                            "]\n"
                            "      <arg>: {<n>|+|-}[,<i>]\n"
                            "        <n>: negative iff its value is greater or equal to <n>\n"
                            "        +  : always positive\n"
                            "        -  : always negative\n"
                            "      <i>  : Only enable for thread <i>"),
                     parser_sign_value(theory->deferred, Target::SignValue), true);
            opts.add(group, "refine-reasons",
                     format("Refine reasons during propagation [",
                            flag_str(config.default_solver_config.refine_reasons),
                            "]\n"
                            "      <arg>: {yes|no}[,<i>]\n"
                            "      <i>  : Only enable for thread <i>"),
                     parser_bool_thread(theory->deferred, Target::RefineReasons), true);
            opts.add(group, "refine-introduce",
                     format("Introduce order literals when generating reasons [",
                            flag_str(config.default_solver_config.refine_introduce),
                            "]\n"
                            "      <arg>: {yes|no}[,<i>]\n"
                            "      <i>  : Only enable for thread <i>"),
                     parser_bool_thread(theory->deferred, Target::RefineIntroduce), true);
            opts.add(group, "propagate-chain",
                     format("Use closest order literal as reason [",
                            flag_str(config.default_solver_config.propagate_chain),
                            "]\n"
                            "      <arg>: {yes|no}[,<i>]\n"
                            "      <i>  : Only enable for thread <i>"),
                     parser_bool_thread(theory->deferred, Target::PropagateChain), true);
            opts.add(group, "split-all",
                     format("Split all domains on total assignment [", flag_str(config.default_solver_config.split_all),
                            "]\n"
                            "      <arg>: {yes|no}[,<i>]\n"
                            "      <i>  : Only enable for thread <i>"),
                     parser_bool_thread(theory->deferred, Target::SplitAll), true);

            // hidden/debug
            opts.add(group, "@2,min-int", format("Set minimum integer [", config.min_int, "]"),
                     parser_num<val_t>(config.min_int, MIN_VAL, MAX_VAL), false, "<i>");
            opts.add(group, "@2,max-int", format("Set maximum integer [", config.max_int, "]"),
                     parser_num<val_t>(config.max_int, MIN_VAL, MAX_VAL), false, "<i>");
            opts.add_flag(group, "@2,check-solution",
                          format("Verify solutions [", flag_str(config.check_solution), "]"), config.check_solution);
            opts.add_flag(group, "@2,check-state",
                          format("Check state invariants [", flag_str(config.check_state), "]"), config.check_state);
        }
        CLINGO_CATCH;
    }

    static auto validate_options(void *self) -> bool {
        CLINGO_TRY {
            auto *theory = static_cast<clingcon_theory *>(self);
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
        CLINGO_CATCH;
    }

    static auto on_model(void *self, clingo_model_t *model) -> bool {
        CLINGO_TRY {
            auto *theory = static_cast<clingcon_theory *>(self);
            Clingo::Model m{model};
            theory->propagator.on_model(m);
        }
        CLINGO_CATCH;
    }

    static auto lookup_symbol(void *self, clingo_symbol_t symbol, size_t *index, bool *found) -> bool {
        CLINGO_TRY {
            auto *theory = static_cast<clingcon_theory *>(self);
            auto res = theory->propagator.get_index(Clingo::Symbol{symbol, true});
            if (found != nullptr) {
                *found = res.has_value();
            }
            if (res && index != nullptr) {
                *index = *res + 1;
            }
        }
        CLINGO_CATCH;
    }

    // static auto clingcon_get_symbol(clingcon_theory *theory, size_t index) -> clingo_symbol_t {
    //     auto sym = theory->propagator.get_symbol(index - 1);
    //     assert(sym.has_value());
    //     return sym->to_c();
    // }

    static auto assignment_next(void *self, [[maybe_unused]] uint32_t thread_id, bool *init, size_t *index,
                                bool *has_value) -> bool {
        CLINGO_TRY {
            auto *theory = static_cast<clingcon_theory *>(self);
            if (std::exchange(*init, false)) {
                *index = 0;
            }
            auto const &map = theory->propagator.var_map();
            auto it = map.lower_bound(static_cast<var_t>(*index));
            *has_value = it != map.end();
            if (*has_value) {
                *index = *index + 1;
            }
        }
        CLINGO_CATCH;
    }

    static auto assignment_get_value(void *self, uint32_t thread_id, size_t index, clingo_symbol_t *symbol,
                                     clingo_theory_value_t *value, bool *has_value) -> bool {
        CLINGO_TRY {
            auto *theory = static_cast<clingcon_theory *>(self);
            auto sym = theory->propagator.get_symbol(static_cast<var_t>(index - 1));
            if (has_value != nullptr) {
                *has_value = sym.has_value();
            }
            if (sym) {
                if (symbol != nullptr) {
                    *symbol = c_cast(*sym);
                    clingo_symbol_acquire(*symbol);
                }
                if (value != nullptr) {
                    value->type = clingo_theory_value_type_int; // NOLINT
                    value->int_number =
                        theory->propagator.get_value(static_cast<var_t>(index - 1), thread_id); // NOLINT
                }
            }
        }
        CLINGO_CATCH;
    }

    static auto on_statistics(void *self, clingo_stats_t *stats) -> bool {
        CLINGO_TRY {
            auto *theory = static_cast<clingcon_theory *>(self);
            uint64_t root = 0;
            handle_error(clingo_stats_root(stats, &root));
            auto cpp_stats = Clingo::Stats{stats, root};
            theory->propagator.on_statistics(cpp_stats["user_step"], cpp_stats["user_accu"]);
        }
        CLINGO_CATCH;
    }

    Clingo::Library lib;
    Propagator propagator{lib};
    Clingo::Options::ParserList parsers;
    Deferred deferred;
    bool shift_constraints{true};
};

} // namespace

extern "C" auto clingcon_create(clingo_lib_t *lib, clingo_theory_t *theory) -> bool {
    CLINGO_TRY {
        *theory = clingo_theory_t{
            clingcon_theory::info,
            clingcon_theory::destroy,
            clingcon_theory::register_,
            clingcon_theory::rewrite_ast,
            clingcon_theory::prepare,
            clingcon_theory::register_options,
            clingcon_theory::validate_options,
            clingcon_theory::configure,
            clingcon_theory::on_model,
            clingcon_theory::on_statistics,
            clingcon_theory::lookup_symbol,
            clingcon_theory::assignment_next,
            clingcon_theory::assignment_get_value,
            nullptr,
        };
        theory->self = std::make_unique<clingcon_theory>(Clingo::Library{lib, true}).release();
    }
    CLINGO_CATCH;
}
