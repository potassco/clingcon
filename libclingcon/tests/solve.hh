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

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <sstream>

using namespace Clingcon;

using S = std::vector<std::string>;
using O = std::vector<std::optional<val_t>>;

class SolveEventHandler : public Clingo::SolveEventHandler {
  public:
    SolveEventHandler(Propagator &p) : p{p} {}
    void on_statistics(Clingo::UserStatistics step, Clingo::UserStatistics accu) override {
        p.on_statistics(step, accu);
    }
    auto on_model(Clingo::Model &model) -> bool override {
        if (model.optimality_proven()) {
            if (!proven) {
                models.clear();
                proven = true;
            }
        } else {
            proven = false;
        }
        p.on_model(model);
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
        for (auto [var, sym] : p.var_map()) {
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
    Propagator &p;
    S models;
    bool proven = false;
};

inline auto create_configs(val_t min_int = Clingcon::DEFAULT_MIN_INT, val_t max_int = Clingcon::DEFAULT_MAX_INT)
    -> std::vector<Config> {
    SolverConfig sconfig{Heuristic::MaxChain, 0, false, true, true, true};
    constexpr uint32_t m = 1000;
    constexpr double r = 1.0;
    constexpr uint64_t f = static_cast<uint64_t>(m) * 10;
    constexpr uint32_t o = std::numeric_limits<uint32_t>::max();
    auto configs = {
        Config{{}, sconfig, 0, 0, 0, 0, 0, min_int, max_int, false, false, false, true, true}, // basic
        Config{{}, sconfig, 0, 0, 0, 0, 0, min_int, max_int, true, false, false, true, true},  // sort constraints
        Config{{}, sconfig, 0, f, m, m, o, min_int, max_int, true, false, false, true, true},  // translate
        Config{
            {}, sconfig, 0, f, m, m, o, min_int, max_int, true, false, true, true, true}, // translate + order clauses
        Config{{}, sconfig, 0, f, m, m, o, min_int, max_int, true, true, false, true, true},  // translate literals only
        Config{{}, sconfig, r, f, 0, m, o, min_int, max_int, true, false, false, true, true}, // translate weight
                                                                                              // constraints
    };
    return configs;
}

inline auto solve(Config const &config, std::string const &prg) -> S {
    Propagator p;
    p.config() = config;
    SolveEventHandler handler{p};

    Clingo::Control ctl{{"100", "--opt-mode=optN", "-t8"}};
    ctl.add("base", {}, THEORY);
    Clingo::AST::with_builder(ctl, [prg](Clingo::AST::ProgramBuilder &builder) {
        Clingo::AST::parse_string(prg.c_str(), [&builder](Clingo::AST::Node const &stm) {
            transform(
                stm, [&builder](Clingo::AST::Node const &stm) { builder.add(stm); }, true);
        });
    });
    ctl.register_propagator(p);
    ctl.ground({{"base", {}}});

    if (ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_interrupted()) {
        throw std::runtime_error("interrupted");
    }
    bool has_minimize = p.has_minimize();
    if (has_minimize && !handler.models.empty()) {
        auto minimize = p.remove_minimize();
        CoVarVec elems;
        elems.reserve(minimize->size());
        for (auto [co, var] : *minimize) {
            elems.emplace_back(co, var);
        }
        val_t bound = static_cast<val_t>(ctl.statistics()["user_step"]["Clingcon"]["Cost"].value());
        p.add_constraint(SumConstraint::create(TRUE_LIT, bound + minimize->adjust(), elems, true));
        handler.models.erase(handler.models.begin(), handler.models.end() - 1);
    }
    std::sort(handler.models.begin(), handler.models.end());

    // NOTE: We test the reversed options using multi-shot solving.
    S models = std::move(handler.models);
    handler.models.clear();
    for (auto &config : p.config().solver_configs) {
        config.split_all = !config.split_all;
        config.refine_introduce = !config.refine_introduce;
        config.refine_reasons = !config.refine_reasons;
        config.propagate_chain = !config.propagate_chain;
    }
    if (ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_interrupted()) {
        throw std::runtime_error("interrupted");
    }
    std::sort(handler.models.begin(), handler.models.end());

    if (!has_minimize || models.empty()) {
        REQUIRE(models == handler.models);
    } else {
        REQUIRE(std::binary_search(handler.models.begin(), handler.models.end(), models.front()));
    }

    return handler.models;
}
inline auto solve(std::string const &prg, val_t min_int = Clingcon::DEFAULT_MIN_INT,
                  val_t max_int = Clingcon::DEFAULT_MAX_INT) -> S {
    std::optional<S> last = std::nullopt;
    int i = 0;
    for (auto const &config : create_configs(min_int, max_int)) {
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
    return *last;
}

inline auto solve_multi(Config const &config, std::string const &prg, Clingo::PartSpan const &parts) -> S {
    Propagator p;
    p.config() = config;
    std::vector<char const *> opts{"0", "-t8"};
    Clingo::Control ctl{opts};
    ctl.add("base", {}, THEORY);
    Clingo::AST::with_builder(ctl, [prg](Clingo::AST::ProgramBuilder &builder) {
        Clingo::AST::parse_string(prg.c_str(), [&builder](Clingo::AST::Node const &stm) {
            transform(
                stm, [&builder](Clingo::AST::Node const &stm) { builder.add(stm); }, true);
        });
    });
    ctl.register_propagator(p);

    S result;
    bool sep = false;
    for (auto const &part : parts) {
        ctl.ground({part});

        SolveEventHandler handler{p};
        if (ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_interrupted()) {
            throw std::runtime_error("interrupted");
        }
        if (sep) {
            result.emplace_back("---");
        } else {
            sep = true;
        }
        std::sort(handler.models.begin(), handler.models.end());
        std::copy(handler.models.begin(), handler.models.end(), std::back_inserter(result));
    }
    return result;
}

inline auto solve_multi(std::string const &prg, Clingo::PartSpan const &parts,
                        val_t min_int = Clingcon::DEFAULT_MIN_INT, val_t max_int = Clingcon::DEFAULT_MAX_INT) -> S {
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
    return *last;
}

inline auto solve_opt(Config const &config, std::string const &prg, Clingo::PartSpan const &parts, bool null_enum)
    -> O {
    Propagator p;
    p.config() = config;
    std::vector<char const *> opts{"0", "-t8"};
    if (null_enum) {
        opts.emplace_back("--enum-mode=user");
    }
    Clingo::Control ctl{opts};
    ctl.add("base", {}, THEORY);
    Clingo::AST::with_builder(ctl, [prg](Clingo::AST::ProgramBuilder &builder) {
        Clingo::AST::parse_string(prg.c_str(), [&builder](Clingo::AST::Node const &stm) {
            transform(
                stm, [&builder](Clingo::AST::Node const &stm) { builder.add(stm); }, true);
        });
    });
    ctl.register_propagator(p);

    O bounds;
    for (auto const &part : parts) {
        ctl.ground({part});

        SolveEventHandler handler{p};
        if (ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_interrupted()) {
            throw std::runtime_error("interrupted");
        }
        std::optional<val_t> bound;
        auto stat = ctl.statistics()["user_step"]["Clingcon"];
        if (stat.has_subkey("Cost")) {
            bound = static_cast<val_t>(stat["Cost"].value());
        } else {
            stat = ctl.statistics()["summary"];
            if (stat.has_subkey("costs")) {
                bound = static_cast<val_t>(stat["costs"][size_t(0)].value());
            }
        }
        bounds.emplace_back(bound);
    }
    return bounds;
}

inline auto solve_opt(std::string const &prg, Clingo::PartSpan const &parts, val_t min_int = Clingcon::DEFAULT_MIN_INT,
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

#endif // CLINGCON_TEST_SOLVE_H
