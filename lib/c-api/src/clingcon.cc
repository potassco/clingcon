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
using namespace std::string_view_literals;

namespace {

constexpr uint32_t MAX_THREADS = 64;

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

auto decide(clingo_assignment_t const *assignment, clingo_literal_t fallback, void *data, clingo_literal_t *result)
    -> bool {
    CLINGO_TRY {
        *result = static_cast<Propagator *>(data)->decide(Clingo::Assignment{assignment}, fallback);
    }
    CLINGO_CATCH;
}

auto flag_str(bool value) -> std::string_view {
    return value ? "yes" : "no";
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

inline auto to_lower(char c) -> char {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

//! Check if two strings are lower case equal.
auto iequals(std::string_view a, std::string_view b) -> bool {
    return a.size() == b.size() && std::ranges::equal(a, b, [](char a, char b) { return to_lower(a) == to_lower(b); });
}

template <typename Enum, size_t N> class EnumStringMap {
  public:
    constexpr EnumStringMap(const std::array<std::pair<std::string_view, Enum>, N> &map) : map_{map} {}

    [[nodiscard]] auto from_string(std::string_view value) const -> Enum {
        for (const auto &[name, mode] : map_) {
            if (iequals(value, name)) {
                return mode;
            }
        }
        throw std::invalid_argument("invalid enum string");
    }

    [[nodiscard]] auto to_string(Enum mode) const -> std::string_view {
        for (const auto &[name, m] : map_) {
            if (mode == m) {
                return name;
            }
        }
        throw std::invalid_argument("invalid enum value");
    }

  private:
    std::array<std::pair<std::string_view, Enum>, N> map_;
};

template <typename Enum, size_t N>
constexpr auto make_enum_string_map(std::pair<std::string_view, Enum> const (&map)[N]) { // NOLINT
    return EnumStringMap<Enum, N>(std::to_array(map));
}

class ConfigClause {
  public:
    ConfigClause(Config &cfg) : cfg_{&cfg} {}

    void set(std::string_view value) {
        auto comma = value.find(',');
        if (comma != std::string_view::npos) {
            cfg_->set_clause_limit_total(parse_num<val_t>(value.substr(comma + 1)));
        }
        cfg_->set_clause_limit(parse_num<uint32_t>(value.substr(0, comma)));
    }

    [[nodiscard]] auto get() const -> std::optional<std::string> {
        std::ostringstream out;
        out << cfg_->clause_limit() << "," << cfg_->clause_limit_total();
        return std::move(out).str();
    }

    static auto desc() -> std::string {
        return format("Restrict translation to clauses [", DEFAULT_CLAUSE_LIMIT, ",", DEFAULT_CLAUSE_LIMIT_TOTAL, "]\n",
                      "      <n>: maximum clauses per constraint\n"
                      "      <m>: maximum clauses total");
    }

  private:
    Config *cfg_;
};

class ConfigBP {
  public:
    ConfigBP(Config &cfg) : cfg_{&cfg} {}

    void set(std::string_view value) { cfg_->set_weight_constraint_ratio(parse_num<double>(value)); }

    [[nodiscard]] auto get() const -> std::optional<std::string> {
        std::ostringstream out;
        out << cfg_->weight_constraint_ratio();
        return std::move(out).str();
    }

    static auto desc() -> std::string {
        return format("Translate to weight constraints if ratio of variables and literals is less equal <r> [",
                      DEFAULT_WEIGHT_CONSTRAINT_RATIO, "]");
    }

  private:
    Config *cfg_;
};

class ConfigDistinct {
  public:
    ConfigDistinct(Config &cfg) : cfg_{&cfg} {}

    void set(std::string_view value) { cfg_->set_distinct_limit(parse_num<uint32_t>(value)); }

    [[nodiscard]] auto get() const -> std::optional<std::string> {
        std::ostringstream out;
        out << cfg_->distinct_limit();
        return std::move(out).str();
    }

    static auto desc() -> std::string {
        return format("Restrict translation of distinct constraints to <n> pb constraints [", DEFAULT_DISTINCT_LIMIT,
                      "]");
    }

  private:
    Config *cfg_;
};

class ConfigOpt {
  public:
    ConfigOpt(Config &cfg) : cfg_{&cfg} {}

    void set(std::string_view value) { cfg_->set_translate_minimize(parse_num<uint32_t>(value)); }

    [[nodiscard]] auto get() const -> std::optional<std::string> {
        std::ostringstream out;
        out << cfg_->translate_minimize();
        return std::move(out).str();
    }

    static auto desc() -> std::string {
        return format("Configure translation of minimize constraint [", DEFAULT_TRANSLATE_MINIMIZE,
                      "]\n"
                      "      <n>: translate if required literals less than <n>\n"
                      "        0  : never translate\n"
                      "        max: always translate");
    }

  private:
    Config *cfg_;
};

class ConfigHeuristic {
  public:
    ConfigHeuristic(Config &cfg) : cfg_{&cfg} {}

    void set(std::optional<size_t> index, std::string_view value) {
        cfg_->set_heuristic(map_.from_string(value), index);
    }

    [[nodiscard]] auto get(std::optional<size_t> index) const -> std::optional<std::string_view> {
        return map_.to_string(cfg_->heuristic(index));
    }

    [[nodiscard]] auto size() const -> size_t { return cfg_->size(); }

    static auto desc() {
        return format("Make the decision heuristic aware of order literals [", map_.to_string(Heuristic::None),
                      "]\n"
                      "      <arg>: {none,max-chain}[,<i>]\n"
                      "        none     : use clasp's heuristic\n"
                      "        max-chain: assign chains of literals\n"
                      "      <i>  : Only enable for thread <i>");
    }

  private:
    static constexpr auto map_ = make_enum_string_map<Heuristic>({
        {"no", Heuristic::None},
        {"max-chain", Heuristic::MaxChain},
    });

    Config *cfg_;
};

class ConfigSign {
  public:
    ConfigSign(Config &cfg) : cfg_{&cfg} {}

    void set(std::optional<size_t> index, std::string_view value) {
        if (value == "+") {
            cfg_->set_sign_value(std::numeric_limits<val_t>::max(), index);
        } else if (value == "-") {
            cfg_->set_sign_value(std::numeric_limits<val_t>::min(), index);
        } else {
            cfg_->set_sign_value(parse_num<val_t>(value), index);
        }
    }

    [[nodiscard]] auto get(std::optional<size_t> index) const -> std::optional<std::string> {
        auto val = cfg_->sign_value(index);
        if (val == std::numeric_limits<val_t>::max()) {
            return "+";
        }
        if (val == std::numeric_limits<val_t>::min()) {
            return "-";
        }
        return std::to_string(val);
    }

    [[nodiscard]] auto size() const -> size_t { return cfg_->size(); }

    static auto desc() {
        return format("Configure the sign of order literals [", DEFAULT_SIGN_VALUE,
                      "]\n"
                      "      <arg>: {<n>|+|-}[,<i>]\n"
                      "        <n>: negative iff its value is greater or equal to <n>\n"
                      "        +  : always positive\n"
                      "        -  : always negative\n"
                      "      <i>  : Only enable for thread <i>");
    }

  private:
    Config *cfg_;
};

auto parse_bool(std::string_view value) -> bool {
    if (iequals(value, "no") || iequals(value, "false") || iequals(value, "off") || iequals(value, "0")) {
        return false;
    }
    if (iequals(value, "yes") || iequals(value, "true") || iequals(value, "on") || iequals(value, "1")) {
        return true;
    }
    throw std::invalid_argument("invalid boolean value");
}

class ConfigReason {
  public:
    ConfigReason(Config &cfg) : cfg_{&cfg} {}

    void set(std::optional<size_t> index, std::string_view value) {
        cfg_->set_refine_reasons(parse_bool(value), index);
    }

    [[nodiscard]] auto get(std::optional<size_t> index) const -> std::optional<std::string_view> {
        return flag_str(cfg_->refine_reasons(index));
    }

    [[nodiscard]] auto size() const -> size_t { return cfg_->size(); }

    static auto desc() {
        return format("Refine reasons during propagation [", flag_str(DEFAULT_REFINE_REASONS),
                      "]\n"
                      "      <arg>: {yes|no}[,<i>]\n"
                      "      <i>  : Only enable for thread <i>");
    }

  private:
    Config *cfg_;
};

class ConfigIntroduce {
  public:
    ConfigIntroduce(Config &cfg) : cfg_{&cfg} {}

    void set(std::optional<size_t> index, std::string_view value) {
        cfg_->set_refine_introduce(parse_bool(value), index);
    }

    [[nodiscard]] auto get(std::optional<size_t> index) const -> std::optional<std::string_view> {
        return flag_str(cfg_->refine_introduce(index));
    }

    [[nodiscard]] auto size() const -> size_t { return cfg_->size(); }

    static auto desc() {
        return format("Introduce order literals when generating reasons [", flag_str(DEFAULT_REFINE_INTRODUCE),
                      "]\n"
                      "      <arg>: {yes|no}[,<i>]\n"
                      "      <i>  : Only enable for thread <i>");
    }

  private:
    Config *cfg_;
};

class ConfigChain {
  public:
    ConfigChain(Config &cfg) : cfg_{&cfg} {}

    void set(std::optional<size_t> index, std::string_view value) {
        cfg_->set_propagate_chain(parse_bool(value), index);
    }

    [[nodiscard]] auto get(std::optional<size_t> index) const -> std::optional<std::string_view> {
        return flag_str(cfg_->propagate_chain(index));
    }

    [[nodiscard]] auto size() const -> size_t { return cfg_->size(); }

    static auto desc() {
        return format("Use closest order literal as reason [", flag_str(DEFAULT_PROPAGATE_CHAIN),
                      "]\n"
                      "      <arg>: {yes|no}[,<i>]\n"
                      "      <i>  : Only enable for thread <i>");
    }

  private:
    Config *cfg_;
};

class ConfigSplit {
  public:
    ConfigSplit(Config &cfg) : cfg_{&cfg} {}

    void set(std::optional<size_t> index, std::string_view value) { cfg_->set_split_all(parse_bool(value), index); }

    [[nodiscard]] auto get(std::optional<size_t> index) const -> std::optional<std::string_view> {
        return flag_str(cfg_->split_all(index));
    }

    [[nodiscard]] auto size() const -> size_t { return cfg_->size(); }

    static auto desc() {
        return format("Split all domains on total assignment [", flag_str(DEFAULT_SPLIT_ALL),
                      "]\n"
                      "      <arg>: {yes|no}[,<i>]\n"
                      "      <i>  : Only enable for thread <i>");
    }

  private:
    Config *cfg_;
};

class ConfigBool {
  public:
    ConfigBool(bool &target) : target_{&target} {}

    void set(std::string_view value) {
        if (iequals(value, "no") || iequals(value, "off") || iequals(value, "0")) {
            *target_ = false;
        } else if (iequals(value, "yes") || iequals(value, "on") || iequals(value, "1")) {
            *target_ = true;
        } else {
            throw std::invalid_argument("invalid boolean value");
        }
    }

    [[nodiscard]] auto get() const -> std::optional<std::string_view> { return flag_str(*target_); }

  private:
    bool *target_;
};

class ConfigMin {
  public:
    ConfigMin(Config &cfg) : cfg_{&cfg} {}

    void set(std::string_view value) { cfg_->set_min_int(parse_num<val_t>(value, MIN_VAL, MAX_VAL)); }

    [[nodiscard]] auto get() const -> std::optional<std::string> {
        std::ostringstream out;
        out << cfg_->min_int();
        return std::move(out).str();
    }

    static auto desc() { return format("Set minimum integer [", DEFAULT_MIN_INT, "]"); }

  private:
    Config *cfg_;
};

class ConfigMax {
  public:
    ConfigMax(Config &cfg) : cfg_{&cfg} {}

    void set(std::string_view value) { cfg_->set_max_int(parse_num<val_t>(value, MIN_VAL, MAX_VAL)); }

    [[nodiscard]] auto get() const -> std::optional<std::string> {
        std::ostringstream out;
        out << cfg_->max_int();
        return std::move(out).str();
    }

    static auto desc() { return format("Set maximum integer [", DEFAULT_MAX_INT, "]"); }

  private:
    Config *cfg_;
};

auto desc_literals() {
    return format("Only create literals during translation but no clauses [", flag_str(DEFAULT_LITERALS_ONLY), "]");
}

auto desc_shift() {
    return "Shift constraints into head of integrity constraints [true]";
}

auto desc_sort() {
    return format("Sort constraint elements [", flag_str(DEFAULT_SORT_CONSTRAINTS), "]");
}

auto desc_order() {
    return format("Add binary clauses for order literals after translation [", flag_str(DEFAULT_ADD_ORDER_CLAUSES),
                  "]");
}

auto desc_solution() {
    return format("Verify solutions [", flag_str(DEFAULT_CHECK_SOLUTION), "]");
}

auto desc_state() {
    return format("Check state invariants [", flag_str(DEFAULT_CHECK_STATE), "]");
}

template <class T> auto parser(Config &config) {
    return [&config](std::string_view value) { T{config}.set(value); };
}

//! Turn the comma separated suffix of value into a thread id.
//!
//! The function returns a nullopt if there is no comma in value.
auto parse_thread(std::string_view value) -> std::pair<std::optional<clingo_id_t>, std::string_view> {
    auto pos = value.find_last_of(',');
    if (pos != std::string_view::npos) {
        auto res = parse_num<clingo_id_t>(value.substr(pos + 1), 0, MAX_THREADS - 1); // NOLINT
        return {res, value.substr(0, pos)};
    }
    return {std::nullopt, value};
}

template <class T> auto parser_thread(Config &config) {
    return [&config](std::string_view value) {
        auto [index, span] = parse_thread(value);
        T{config}.set(index, span);
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
            auto theory_cfg = theory->propagator.config();
            auto ctl = Clingo::Control{control, true};
            auto cnf = ctl.config();
            cnf.add("clingcon", "CSP Options");
            cnf.add("clingcon.shift_constraints", desc_shift(), ConfigBool{theory->shift_constraints});
            cnf.add("clingcon.sort_constraints", desc_sort(), ConfigBool{theory_cfg.sort_constraints});
            cnf.add("clingcon.translate_clauses", ConfigClause::desc(), ConfigClause{theory_cfg});
            cnf.add("clingcon.literals_only", desc_literals(), ConfigBool{theory_cfg.literals_only});
            cnf.add("clingcon.translate_pb", ConfigBP::desc(), ConfigBP{theory_cfg});
            cnf.add("clingcon.translate_distinct", ConfigDistinct::desc(), ConfigDistinct{theory_cfg});
            cnf.add("clingcon.translate_opt", ConfigOpt::desc(), ConfigOpt{theory_cfg});
            cnf.add("clingcon.add_order_clauses", desc_order(), ConfigBool{theory_cfg.add_order_clauses});
            cnf.add("clingcon.min_int", ConfigMin::desc(), ConfigMin{theory_cfg});
            cnf.add("clingcon.max_int", ConfigMax::desc(), ConfigMax{theory_cfg});
            cnf.add("clingcon.check_solution", desc_solution(), ConfigBool{theory_cfg.check_solution});
            cnf.add("clingcon.check_state", desc_state(), ConfigBool{theory_cfg.check_state});
            cnf.add("clingcon.order_heuristic", ConfigHeuristic::desc(), ConfigHeuristic{theory_cfg});
            cnf.add("clingcon.sign_value", ConfigSign::desc(), ConfigSign{theory_cfg});
            cnf.add("clingcon.refine_reasons", ConfigReason::desc(), ConfigReason{theory_cfg});
            cnf.add("clingcon.refine_introduce", ConfigIntroduce::desc(), ConfigIntroduce{theory_cfg});
            cnf.add("clingcon.propagate_chain", ConfigChain::desc(), ConfigChain{theory_cfg});
            cnf.add("clingcon.split_all", ConfigSplit::desc(), ConfigSplit{theory_cfg});
            ctl.parse_string(Clingcon::THEORY);
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

    static auto prepare(void *self, clingo_control_t *control) -> bool {
        CLINGO_TRY {
            if (auto *theory = static_cast<clingcon_theory *>(self); !theory->initialized) {
                static constexpr clingo_propagator_t propagator = {init,  nullptr, propagate, undo,
                                                                   check, nullptr, nullptr};
                static constexpr clingo_propagator_t heuristic = {init,  nullptr, propagate, undo,
                                                                  check, decide,  nullptr};
                theory->initialized = true;
                auto &cnf = theory->propagator.config();
                handle_error(clingo_control_register_propagator(control, cnf.has_heuristic() ? &heuristic : &propagator,
                                                                &theory->propagator));
            }
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

    static auto configure([[maybe_unused]] void *self, [[maybe_unused]] char const *key_data,
                          [[maybe_unused]] size_t key_size, [[maybe_unused]] char const *value_data,
                          [[maybe_unused]] size_t value_size) -> bool {
        CLINGO_TRY {
            throw std::invalid_argument{"unknown config key"};
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
            opts.add_flag(group, "shift-constraints", desc_shift(), theory->shift_constraints);
            opts.add_flag(group, "sort-constraints", desc_sort(), config.sort_constraints);
            opts.add(group, "translate-clauses", ConfigClause::desc(), parser<ConfigClause>(config), false,
                     "<n>[,<m>]");
            opts.add_flag(group, "literals-only", desc_literals(), config.literals_only);
            opts.add(group, "translate-pb", ConfigBP::desc(), parser<ConfigBP>(config), false, "<r>");
            opts.add(group, "translate-distinct", ConfigDistinct::desc(), parser<ConfigDistinct>(config), false, "<n>");
            opts.add(group, "translate-opt", ConfigOpt::desc(), parser<ConfigOpt>(config), false, "<n>");
            opts.add_flag(group, "add-order-clauses", desc_order(), config.add_order_clauses);

            // propagation
            opts.add(group, "order-heuristic", ConfigHeuristic::desc(), parser_thread<ConfigHeuristic>(config), true);
            opts.add(group, "sign-value", ConfigSign::desc(), parser_thread<ConfigSign>(config), true);
            opts.add(group, "refine-reasons", ConfigReason::desc(), parser_thread<ConfigReason>(config), true);
            opts.add(group, "refine-introduce", ConfigIntroduce::desc(), parser_thread<ConfigIntroduce>(config), true);
            opts.add(group, "propagate-chain", ConfigChain::desc(), parser_thread<ConfigChain>(config), true);
            opts.add(group, "split-all", ConfigSplit::desc(), parser_thread<ConfigSplit>(config), true);

            // hidden/debug
            opts.add(group, "@2,min-int", ConfigMin::desc(), parser<ConfigMin>(config), false, "<i>");
            opts.add(group, "@2,max-int", ConfigMax::desc(), parser<ConfigMax>(config), false, "<i>");
            opts.add_flag(group, "@2,check-solution", desc_solution(), config.check_solution);
            opts.add_flag(group, "@2,check-state", desc_state(), config.check_state);
        }
        CLINGO_CATCH;
    }

    static auto validate_options(void *self) -> bool {
        CLINGO_TRY {
            auto *theory = static_cast<clingcon_theory *>(self);
            auto &config = theory->propagator.config();

            if (config.min_int() > config.max_int()) {
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
                    value->type = clingo_theory_value_type_int;
                    // NOLINTNEXTLINE
                    value->int_number = theory->propagator.get_value(static_cast<var_t>(index - 1), thread_id);
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
    bool shift_constraints{true};
    bool initialized{false};
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
