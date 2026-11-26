// {{{ MIT License
//
// Copyright (C) Siemens 2026
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

#include "solve.hh"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

using namespace Clingcon;

namespace {

// Redirects std::cerr into a buffer for the lifetime of the object.
class CaptureCerr {
  public:
    CaptureCerr() : old_{std::cerr.rdbuf(buffer_.rdbuf())} {}
    CaptureCerr(CaptureCerr const &) = delete;
    CaptureCerr(CaptureCerr &&) = delete;
    auto operator=(CaptureCerr const &) -> CaptureCerr & = delete;
    auto operator=(CaptureCerr &&) -> CaptureCerr & = delete;
    ~CaptureCerr() { std::cerr.rdbuf(old_); }

    [[nodiscard]] auto str() const -> std::string { return buffer_.str(); }

  private:
    std::ostringstream buffer_;
    std::streambuf *old_;
};

void add_program(Clingo::Control &ctl, std::string const &prg) {
    ctl.add("base", {}, THEORY);
    Clingo::AST::with_builder(ctl, [prg](Clingo::AST::ProgramBuilder &builder) -> void {
        Clingo::AST::parse_string(prg.c_str(), [&builder](Clingo::AST::Node const &stm) -> void {
            transform(stm, [&builder](Clingo::AST::Node const &stm) -> void { builder.add(stm); }, true);
        });
    });
    ctl.ground({{"base", {}}});
}

// Drives the solve calls like the application does. Returns the models in the
// order they are found; the last one is the optimum.
auto solve_steps(Config const &config, std::string const &prg, size_t *calls = nullptr) -> S {
    Propagator p;
    p.config() = config;
    p.enable_steps();
    SolveEventHandler handler{p};
    Clingo::Control ctl{{"0", "-t1"}};
    ctl.register_propagator(p);
    add_program(ctl, prg);

    size_t solve_calls = 0;
    bool exhausted = false;
    do {
        ++solve_calls;
        auto result = ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get();
        if (result.is_interrupted()) {
            throw std::runtime_error("interrupted");
        }
        exhausted = result.is_exhausted();
    } while (p.step(exhausted));

    if (calls != nullptr) {
        *calls = solve_calls;
    }
    return handler.models;
}

// The same for a program with optimization levels.
auto solve_lex(std::string const &prg, BoundStep step) -> S {
    auto config = create_configs(0, 100).front();
    config.lex_opt = LexOptMode::MultiShot;
    config.bound_step = step;
    config.hybrid_k = 2;
    return solve_steps(config, prg);
}

// The optimum of this program is 40. The bound follows from the rules that
// derive the sum constraint and not from the domain of x, so the lower bound a
// bisection starts from is 0 and every midpoint below 40 is refuted.
constexpr char const *GAP_PROGRAM = "&dom{0..100} = x. { a }. "
                                    "&sum{ x } >= 40 :- a. &sum{ x } >= 40 :- not a. "
                                    "&minimize{ x }.";

// The lexicographic optimum is reached only after the first level is locked:
// z >= 42-x pushes x to 37 once x+y is fixed at its optimum.
constexpr char const *LEX_PROGRAM = "&dom { 0..50 } = x. &dom { 0..50 } = y. &dom { 0..50 } = z. "
                                    "&sum { x; y } >= 37. &sum { z; x } >= 42. "
                                    "&minimize { x@2; y@2 }. &minimize { z@1 }.";

} // namespace

TEST_CASE("bound-step-constant", "[solving]") {
    // The optimum of 3x+2y is 725 at x=125 and y=175. A bisection needs
    // several solve calls here, and each one rebuilds the minimize
    // constraint; the constant of the objective has to survive that.
    constexpr char const *PROGRAM = "&dom{0..1000} = x. &dom{0..1000} = y. "
                                    "&sum{ x; y } >= 300. &sum{ y; -1*x } <= 50. ";
    constexpr int OPTIMUM = 725;
    for (auto step : {BoundStep::Linear, BoundStep::Bisect, BoundStep::Hybrid}) {
        for (auto constant : {-200, -11, 11, 200}) {
            auto config = create_configs(0, 1000).front();
            config.bound_step = step;
            config.hybrid_k = 16;

            Propagator p;
            p.config() = config;
            p.enable_steps();
            SolveEventHandler handler{p};
            Clingo::Control ctl{{"0", "-t1"}};
            ctl.register_propagator(p);
            add_program(ctl, std::string{PROGRAM} + "&minimize{ 3*x + 2*y + " + std::to_string(constant) + " }.");

            std::optional<double> cost;
            bool exhausted = false;
            do {
                exhausted = ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_exhausted();
                if (auto stats = ctl.statistics()["user_step"]["Clingcon"]; stats.has_subkey("Cost")) {
                    cost = stats["Cost"].value();
                }
            } while (p.step(exhausted));
            REQUIRE(handler.models.back() == "x=125 y=175");
            REQUIRE(cost.has_value());
            REQUIRE(*cost == OPTIMUM + constant);
        }
    }
}

TEST_CASE("bound-step", "[solving]") {
    SECTION("linear is the default") {
        auto config = create_configs(0, 100).front();
        REQUIRE(config.bound_step == BoundStep::Linear);
        REQUIRE(solve(config, "&dom{0..10} = x. &minimize{ x }.") == S({"x=0"}));
    }
    SECTION("bisect finds the same optimum as linear") {
        constexpr char const *prg = "&dom{0..50} = x. &dom{0..50} = y. "
                                    "&sum{ x; y } >= 25. &minimize{ x }.";
        auto config = create_configs(0, 100).front();
        auto models = solve(config, prg);
        REQUIRE(!models.empty());
        REQUIRE(models.front() == "x=0 y=25");
        config.bound_step = BoundStep::Bisect;
        REQUIRE(solve(config, prg) == models);
    }
    SECTION("hybrid finds the same optimum as linear") {
        constexpr char const *prg = "&dom{0..50} = x. &dom{0..50} = y. "
                                    "&sum{ x; y } >= 25. &minimize{ x }.";
        auto config = create_configs(0, 100).front();
        auto models = solve(config, prg);
        config.bound_step = BoundStep::Hybrid;
        config.hybrid_k = 5;
        REQUIRE(!models.empty());
        REQUIRE(solve(config, prg) == models);
    }
    SECTION("bisect with a negative coefficient") {
        // &minimize{ -1*x } maximizes x.
        auto config = create_configs(0, 50).front();
        config.bound_step = BoundStep::Bisect;
        REQUIRE(solve(config, "&dom{0..50} = x. &minimize{ -1*x }.") == S({"x=50"}));
    }
    SECTION("a small k steps linearly") {
        auto config = create_configs(0, 100).front();
        config.bound_step = BoundStep::Hybrid;
        config.hybrid_k = 1;
        REQUIRE(solve(config, "&dom{0..20} = x. &minimize{ x }.") == S({"x=0"}));
    }
    SECTION("a large k bisects throughout") {
        auto config = create_configs(0, 100).front();
        config.bound_step = BoundStep::Hybrid;
        config.hybrid_k = 1000;
        REQUIRE(solve(config, "&dom{0..20} = x. &minimize{ x }.") == S({"x=0"}));
    }
    SECTION("all modes find the same optimum") {
        constexpr char const *prg = "&dom{0..30} = x. &dom{0..30} = y. &dom{0..30} = z. "
                                    "&sum{ x; y; z } = 30. &minimize{ x }.";
        auto linear = create_configs(0, 100).front();
        auto bisect = linear;
        bisect.bound_step = BoundStep::Bisect;
        auto hybrid = linear;
        hybrid.bound_step = BoundStep::Hybrid;
        hybrid.hybrid_k = 3;
        auto models = solve(linear, prg);
        REQUIRE(!models.empty());
        REQUIRE(models.front().substr(0, 3) == "x=0");
        REQUIRE(solve(bisect, prg) == models);
        REQUIRE(solve(hybrid, prg) == models);
    }
    SECTION("a linear step proves optimality in one solve call") {
        auto config = create_configs(0, 100).front();
        size_t calls = 0;
        auto models = solve_steps(config, GAP_PROGRAM, &calls);
        REQUIRE(!models.empty());
        REQUIRE(models.back().find("x=40") != std::string::npos);
        REQUIRE(calls == 1);
    }
    SECTION("bisect continues after a refuted bound") {
        // A solve call that ends because no model meets the midpoint bound
        // does not prove optimality; the values between the midpoint and the
        // last model are searched in a further solve call.
        auto config = create_configs(0, 100).front();
        config.bound_step = BoundStep::Bisect;
        size_t calls = 0;
        auto models = solve_steps(config, GAP_PROGRAM, &calls);
        REQUIRE(!models.empty());
        REQUIRE(models.back().find("x=40") != std::string::npos);
        REQUIRE(calls > 1);
    }
    SECTION("hybrid continues after a refuted bound") {
        auto config = create_configs(0, 100).front();
        config.bound_step = BoundStep::Hybrid;
        config.hybrid_k = 2;
        size_t calls = 0;
        auto models = solve_steps(config, GAP_PROGRAM, &calls);
        REQUIRE(!models.empty());
        REQUIRE(models.back().find("x=40") != std::string::npos);
        REQUIRE(calls > 1);
    }
    SECTION("a further solve call finds a better model") {
        // The cap on x follows from a rule, so the lower bound of the
        // minimize constraint is the one the domain of x gives and the first
        // midpoints are within reach.
        constexpr char const *prg = "&dom{0..200} = x. { a }. "
                                    "&sum{ x } <= 120 :- a. &sum{ x } <= 120 :- not a. "
                                    "&minimize{ -1*x }.";
        auto config = create_configs(0, 200).front();
        config.bound_step = BoundStep::Bisect;
        size_t calls = 0;
        auto models = solve_steps(config, prg, &calls);
        REQUIRE(!models.empty());
        REQUIRE(models.back().find("x=120") != std::string::npos);
        REQUIRE(calls > 1);
    }
    SECTION("a solve call cut short is not a refutation") {
        // With a limit of one model the solve call ends after the first
        // model. The midpoint bound it posted has not been refuted, so the
        // loop ends and nothing is raised.
        auto config = create_configs(0, 100).front();
        config.bound_step = BoundStep::Bisect;
        Propagator p;
        p.config() = config;
        p.enable_steps();
        SolveEventHandler handler{p};
        Clingo::Control ctl{{"1", "-t1"}};
        ctl.register_propagator(p);
        add_program(ctl, GAP_PROGRAM);
        auto result = ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get();
        REQUIRE(!result.is_exhausted());
        REQUIRE(!p.step(result.is_exhausted()));
        REQUIRE(handler.models.size() == 1);
    }
    SECTION("bisect without steps is rejected") {
        auto config = create_configs(0, 100).front();
        config.bound_step = BoundStep::Bisect;
        Propagator p;
        p.config() = config;
        SolveEventHandler handler{p};
        Clingo::Control ctl{{"0", "-t1"}};
        ctl.register_propagator(p);
        add_program(ctl, "&dom{0..10} = x. &minimize{ x }.");
        REQUIRE_THROWS(ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get());
    }
    SECTION("a missing step is reported") {
        CaptureCerr capture;
        {
            auto config = create_configs(0, 100).front();
            config.bound_step = BoundStep::Bisect;
            Propagator p;
            p.config() = config;
            p.enable_steps();
            SolveEventHandler handler{p};
            Clingo::Control ctl{{"0", "-t1"}};
            ctl.register_propagator(p);
            add_program(ctl, GAP_PROGRAM);
            static_cast<void>(ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get());
        }
        REQUIRE(capture.str().find("clingcon_step") != std::string::npos);
    }
}

TEST_CASE("bound-step-levels", "[solving]") {
    SECTION("a level whose bounds on level zero rule out the midpoint") {
        // After the first two levels are fixed, x and y are fixed as well and
        // the third level has exactly one value. A continued solve call must
        // not start with a bound below it.
        constexpr char const *prg = "&dom{0..6} = x. &dom{0..6} = y. &sum{ x; -2*y } > 3. "
                                    "&minimize{ -1*x@3 }. &minimize{ -2*x@2; 2*y@2 }. &minimize{ 2*x@1; y@1 }.";
        for (auto step : {BoundStep::Bisect, BoundStep::Hybrid}) {
            auto models = solve_lex(prg, step);
            REQUIRE(!models.empty());
            REQUIRE(models.back() == "x=6 y=0");
        }
    }
    SECTION("a continued solve call whose bound the facts violate") {
        // The program has one model, and the facts that fix it are only
        // derived once a solve call has started. The continued solve call
        // then starts with a bound below the objective's only value; it has
        // to end unsatisfiable, not with an error.
        constexpr char const *prg =
            "&dom{0..3} = x. &dom{0..3} = y. &sum{ x; 2*y } = 2. &sum{ 2*x; -1*y } > -1. "
            "&minimize{ 2*x@3; 2*y@3 }. &minimize{ -2*y@2; 2*x@2 }. &minimize{ -2*x@1; 2*y@1 }.";
        for (auto step : {BoundStep::Bisect, BoundStep::Hybrid}) {
            auto models = solve_lex(prg, step);
            REQUIRE(!models.empty());
            REQUIRE(models.back() == "x=2 y=0");
        }
    }
    SECTION("the last solve call reports the optimum") {
        // The refuted midpoints leave the last model as the optimum; one
        // more solve call reports it, so the call that ends the search ends
        // with a model.
        auto config = create_configs(0, 100).front();
        config.bound_step = BoundStep::Bisect;
        size_t calls = 0;
        auto models = solve_steps(config, GAP_PROGRAM, &calls);
        REQUIRE(calls > 2);
        REQUIRE(models.size() >= 2);
        REQUIRE(models.back() == models[models.size() - 2]);
        REQUIRE(models.back().find("x=40") != std::string::npos);
    }
    SECTION("bisect bound stepping") {
        auto models = solve_lex(LEX_PROGRAM, BoundStep::Bisect);
        REQUIRE(!models.empty());
        REQUIRE(models.back() == "x=37 y=0 z=5");
    }
    SECTION("hybrid bound stepping") {
        auto models = solve_lex(LEX_PROGRAM, BoundStep::Hybrid);
        REQUIRE(!models.empty());
        REQUIRE(models.back() == "x=37 y=0 z=5");
    }
}
