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

#include <clingcon/parsing.hh>
#include <clingcon/propagator.hh>

#include <clingo/ast.hh>
#include <clingo/control.hh>
#include <clingo/core.hh>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>
#include <sstream>
#include <string_view>

using namespace Clingcon;

using S = std::vector<std::string>;
using O = std::vector<std::optional<val_t>>;

class SolveEventHandler : public Clingo::SolveEventHandler {
  public:
    SolveEventHandler(Propagator &p) : p{&p} {}
    void do_stats(Clingo::Stats step, Clingo::Stats accu) override { p->on_statistics(step, accu); }
    auto do_model(Clingo::Model model) -> bool override {
        if (model.optimality_proven()) {
            if (!proven) {
                models.clear();
                proven = true;
            }
        } else {
            proven = false;
        }
        p->on_model(model);
        std::ostringstream oss;
        bool sep = false;
        std::vector<Clingo::Symbol> symbols = model.symbols();
        std::ranges::sort(symbols);
        for (auto &sym : symbols) {
            if (!sym.match("__csp", 2) && !sym.match("__csp_cost", 1)) {
                if (sep) {
                    oss << " ";
                }
                sep = true;
                oss << sym;
            }
        }
        std::vector<std::pair<Clingo::Symbol, val_t>> assignment;
        for (auto const &[var, sym] : p->var_map()) {
            if (p->shown(var)) {
                assignment.emplace_back(sym, p->get_value(var, model.thread_id()));
            }
        }
        std::ranges::sort(assignment);
        for (auto const &[sym, val] : assignment) {
            if (sep) {
                oss << " ";
            }
            sep = true;
            oss << sym << "=" << val;
        }
        models.emplace_back(oss.str());
        return true;
    }
    Propagator *p;
    S models;
    bool proven = false;
};

inline auto make_config(double weight_constraint_ratio, uint64_t clause_limit_total, uint32_t clause_limit,
                        uint32_t distinct_limit, uint32_t translate_minimize, val_t min_int, val_t max_int,
                        bool sort_constraints, bool literals_only, bool add_order_clauses) {
    auto cfg = Config{};
    cfg.set_heuristic(Heuristic::MaxChain);
    cfg.set_sign_value(0);
    cfg.set_split_all(false);
    cfg.set_propagate_chain(true);
    cfg.set_refine_reasons(true);
    cfg.set_refine_introduce(true);

    cfg.set_weight_constraint_ratio(weight_constraint_ratio);
    cfg.set_clause_limit_total(clause_limit_total);
    cfg.set_clause_limit(clause_limit);
    cfg.set_distinct_limit(distinct_limit);
    cfg.set_translate_minimize(translate_minimize);
    cfg.set_min_int(min_int);
    cfg.set_max_int(max_int);

    cfg.sort_constraints = sort_constraints;
    cfg.literals_only = literals_only;
    cfg.add_order_clauses = add_order_clauses;
    cfg.check_solution = true;
    cfg.check_state = true;
    return cfg;
}

inline auto create_configs(val_t min_int = Clingcon::DEFAULT_MIN_INT, val_t max_int = Clingcon::DEFAULT_MAX_INT)
    -> std::vector<Config> {
    constexpr uint32_t m = 1000;
    constexpr double r = 1.0;
    constexpr uint64_t f = static_cast<uint64_t>(m) * 10;
    constexpr uint32_t o = std::numeric_limits<uint32_t>::max();
    return {
        make_config(0, 0, 0, 0, 0, min_int, max_int, false, false, false), // basic
        make_config(0, 0, 0, 0, 0, min_int, max_int, true, false, false),  // sort constraints
        make_config(0, f, m, m, o, min_int, max_int, true, false, false),  // translate
        make_config(0, f, m, m, o, min_int, max_int, true, false, true),   // translate + order clauses
        make_config(0, f, m, m, o, min_int, max_int, true, true, false),   // translate literals only
        make_config(r, f, 0, m, o, min_int, max_int, true, false, false),  // translate weight constraints
    };
}

struct Fixture {
    Clingo::Library lib;

    auto solve(Config &config, std::string const &str) -> S {

        Clingo::Control ctl{lib, {"100", "--opt-mode=optN", "-t8"}};
        auto &prp = ctl.register_propagator(std::make_unique<Propagator>(lib));
        prp.config() = config;
        auto hnd = SolveEventHandler{prp};

        ctl.parse_string(THEORY);
        auto prg = Clingo::AST::Program{lib};
        Clingo::AST::parse(lib, str, [&](auto const &stm) {
            Clingcon::transform(lib, stm, [&](Clingo::AST::Node const &stm) { prg.add(stm); }, true);
        });
        ctl.join(prg);

        ctl.ground();

        if (ctl.solve({}, std::ref(hnd)).interrupted()) {
            throw std::runtime_error("interrupted");
        }
        bool has_minimize = prp.has_minimize();
        if (has_minimize && !hnd.models.empty()) {
            auto minimize = prp.remove_minimize();
            CoVarVec elems;
            elems.reserve(minimize->size());
            for (auto [co, var] : *minimize) {
                elems.emplace_back(co, var);
            }
            val_t bound = static_cast<val_t>(ctl.stats()["user_step"]["Clingcon"]["Cost"].value());
            prp.add_constraint(SumConstraint::create(TRUE_LIT, bound + minimize->adjust(), elems, true));
            hnd.models.erase(hnd.models.begin(), hnd.models.end() - 1);
        }
        std::ranges::sort(hnd.models);

        // NOTE: We test the reversed options using multi-shot solving.
        S models = std::move(hnd.models);
        hnd.models.clear();
        for (size_t i = 0; i != prp.config().size(); ++i) {
            config.set_split_all(!config.split_all(i));
            config.set_refine_introduce(!config.refine_introduce());
            config.set_refine_reasons(!config.refine_reasons());
            config.set_propagate_chain(!config.propagate_chain());
        }
        if (ctl.solve({}, std::ref(hnd)).interrupted()) {
            throw std::runtime_error("interrupted");
        }
        std::ranges::sort(hnd.models);

        if (!has_minimize || models.empty()) {
            REQUIRE(models == hnd.models);
        } else {
            REQUIRE(std::ranges::binary_search(hnd.models, models.front()));
        }

        return hnd.models;
    }
    auto solve(std::string const &prg, val_t min_int = Clingcon::DEFAULT_MIN_INT,
               val_t max_int = Clingcon::DEFAULT_MAX_INT) -> S {
        std::optional<S> last = std::nullopt;
        int i = 0;
        for (auto &config : create_configs(min_int, max_int)) {
            std::ostringstream oss;
            oss << "configuration: " << i++ << "\nprogram: " << prg;
            INFO(oss.str());
            auto current = solve(config, prg);
            if (last.has_value()) {
                INFO(oss.str());
                REQUIRE(current == *last);
            }
            last = current;
        }
        return last.value(); // NOLINT
    }

    auto solve_multi(Config const &config, std::string const &str, Clingo::PartList parts) -> S {
        std::vector<std::string_view> opts{"0", "-t8"};
        Clingo::Control ctl{lib, opts};
        auto &prp = ctl.register_propagator(std::make_unique<Propagator>(lib));
        prp.config() = config;
        ctl.parse_string(THEORY);

        auto prg = Clingo::AST::Program{lib};
        Clingo::AST::parse(lib, str, [&](auto const &stm) {
            Clingcon::transform(lib, stm, [&](Clingo::AST::Node const &stm) { prg.add(stm); }, true);
        });
        ctl.join(prg);

        S result;
        bool sep = false;
        for (auto const &part : parts) {
            ctl.ground({part});

            SolveEventHandler seh{prp};
            if (ctl.solve({}, std::ref(seh)).interrupted()) {
                throw std::runtime_error("interrupted");
            }
            if (sep) {
                result.emplace_back("---");
            } else {
                sep = true;
            }
            std::ranges::sort(seh.models);
            std::ranges::copy(seh.models, std::back_inserter(result));
        }
        return result;
    }

    auto solve_multi(std::string const &prg, Clingo::PartList parts, val_t min_int = Clingcon::DEFAULT_MIN_INT,
                     val_t max_int = Clingcon::DEFAULT_MAX_INT) -> S {
        std::optional<S> last = std::nullopt;
        int i = 0;
        for (auto const &config : create_configs(min_int, max_int)) {
            std::ostringstream oss;
            oss << "configuration: " << i++ << "\nprogram: " << prg;
            INFO(oss.str());
            auto current = solve_multi(config, prg, parts);
            if (last.has_value()) {
                INFO(oss.str());
                REQUIRE(current == *last);
            }
            last = current;
        }
        return last.value(); // NOLINT
    }

    auto solve_opt(Config const &config, std::string const &str, Clingo::PartList parts, bool null_enum) -> O {
        std::vector<std::string_view> opts{"0", "-t8"};
        if (null_enum) {
            opts.emplace_back("--enum-mode=user");
        }
        Clingo::Control ctl{lib, opts};
        auto &p = ctl.register_propagator(std::make_unique<Propagator>(lib));
        p.config() = config;

        ctl.parse_string(THEORY);
        auto prg = Clingo::AST::Program{lib};
        Clingo::AST::parse(lib, str, [&](auto const &stm) {
            Clingcon::transform(lib, stm, [&](Clingo::AST::Node const &stm) { prg.add(stm); }, true);
        });
        ctl.join(prg);

        O bounds;
        for (auto const &part : parts) {
            ctl.ground({part});

            SolveEventHandler handler{p};
            if (ctl.solve({}, std::ref(handler)).interrupted()) {
                throw std::runtime_error("interrupted");
            }
            std::optional<val_t> bound;
            auto stat = ctl.stats()["user_step"]["Clingcon"].map();
            if (stat.contains("Cost")) {
                bound = static_cast<val_t>(stat["Cost"].value());
            } else {
                stat = ctl.stats()["summary"].map();
                if (stat.contains("costs")) {
                    bound = static_cast<val_t>(stat["costs"][size_t(0)].value());
                }
            }
            bounds.emplace_back(bound);
        }
        return bounds;
    }

    auto solve_opt(std::string const &prg, Clingo::PartList parts, val_t min_int = Clingcon::DEFAULT_MIN_INT,
                   val_t max_int = Clingcon::DEFAULT_MAX_INT) -> O {
        int i = 0;
        O bounds;
        for (auto const &config : create_configs(min_int, max_int)) {
            std::ostringstream oss;
            oss << "configuration: " << i << "\nprogram: " << prg;
            INFO(oss.str());
            INFO("  with backtracking enumerator");
            auto current = solve_opt(config, prg, parts, false);
            if (i == 0) {
                bounds = std::move(current);
            } else {
                REQUIRE(bounds == current);
            }
            ++i;
            INFO("  with null enumerator");
            current = solve_opt(config, prg, parts, true);
            REQUIRE(bounds == current);
        }
        return bounds;
    }
};

#endif // CLINGCON_TEST_SOLVE_H
