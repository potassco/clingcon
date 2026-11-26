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

// What a run of solve_lex observed besides the models.
struct LexReport {
    bool multi_level{false};      //!< whether the program was optimized level by level
    size_t calls{0};              //!< the number of solve calls
    std::optional<double> levels; //!< the "Levels" statistic of the last solve call
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
// order they are found; the last one is the lexicographic optimum.
auto solve_lex(Config const &config, std::string const &prg, LexReport *report = nullptr) -> S {
    Propagator p;
    p.config() = config;
    p.enable_steps();
    SolveEventHandler handler{p};
    Clingo::Control ctl{{"0", "-t1"}};
    ctl.register_propagator(p);
    add_program(ctl, prg);

    size_t calls = 0;
    bool exhausted = false;
    do {
        ++calls;
        auto result = ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get();
        if (result.is_interrupted()) {
            throw std::runtime_error("interrupted");
        }
        exhausted = result.is_exhausted();
    } while (p.step(exhausted));

    if (report != nullptr) {
        report->multi_level = p.has_multi_level();
        report->calls = calls;
        auto stats = ctl.statistics()["user_step"]["Clingcon"];
        if (stats.has_subkey("Optimization")) {
            report->levels = stats["Optimization"]["Levels"].value();
        }
    }
    return handler.models;
}

auto solve_lex(std::string const &prg, LexReport *report = nullptr) -> S {
    auto config = create_configs(0, 100).front();
    config.lex_opt = LexOptMode::MultiShot;
    return solve_lex(config, prg, report);
}

// A program whose lexicographic optimum is reached only after the first level
// is locked: z >= 42-x pushes x to 37 once x+y is fixed at its optimum.
constexpr char const *LEX_PROGRAM = "&dom { 0..50 } = x. &dom { 0..50 } = y. &dom { 0..50 } = z. "
                                    "&sum { x; y } >= 37. &sum { z; x } >= 42. "
                                    "&minimize { x@2; y@2 }. &minimize { z@1 }.";

// Two levels with a small search space. The lexicographic optimum minimizes x
// first and y second.
constexpr char const *SMALL_PROGRAM = "&dom{0..5} = x. &dom{0..5} = y. &sum{ x; y } >= 5. &minimize{ x@2; y@1 }.";

} // namespace

TEST_CASE("priority-parsing", "[parsing]") {
    SECTION("single level") {
        auto result = solve("&dom{0..10} = x. &minimize{ x @1 }.");
        REQUIRE(!result.empty());
        REQUIRE(std::find(result.begin(), result.end(), "x=0") != result.end());
    }
    SECTION("two levels") {
        auto result = solve("&dom{0..5} = x. &dom{0..5} = y. "
                            "&minimize{ x @2 }. &minimize{ y @1 }.");
        REQUIRE(!result.empty());
        REQUIRE(std::find(result.begin(), result.end(), "x=0 y=0") != result.end());
    }
    SECTION("level zero") {
        auto result = solve("&dom{0..5} = x. &minimize{ x @0 }.");
        REQUIRE(!result.empty());
        REQUIRE(std::find(result.begin(), result.end(), "x=0") != result.end());
    }
    SECTION("several variables on one level") {
        auto result = solve("&dom{0..5} = x. &dom{0..5} = y. "
                            "&minimize{ x @1 }. &minimize{ y @1 }.");
        REQUIRE(!result.empty());
        REQUIRE(std::find(result.begin(), result.end(), "x=0 y=0") != result.end());
    }
    SECTION("negative coefficient") {
        auto result = solve("&dom{0..5} = x. &minimize{ -1*x @1 }.");
        REQUIRE(!result.empty());
        REQUIRE(std::find(result.begin(), result.end(), "x=5") != result.end());
    }
    SECTION("level outside an objective") {
        REQUIRE_THROWS(solve("&dom{0..5} = x. &sum{ x @1 } >= 1."));
        REQUIRE_THROWS(solve("&dom{0..5} = x@1."));
        REQUIRE_THROWS(solve("&dom{0..5} = x. &show{ x@1 }."));
    }
    SECTION("single level with constraint") {
        auto result = solve("&dom{0..10} = x. &sum{ x } >= 3. &minimize{ x @1 }.");
        REQUIRE(!result.empty());
        REQUIRE(std::find(result.begin(), result.end(), "x=3") != result.end());
    }
}

TEST_CASE("minimize-wide-rhs", "[solving]") {
    SECTION("bound beyond the value range") { REQUIRE(solve("&dom{0..1000} = x. &minimize{ x }.") == S({"x=0"})); }
    SECTION("large coefficient") {
        auto result = solve("&dom{0..10} = x. &minimize{ 100*x }.");
        REQUIRE(!result.empty());
        REQUIRE(std::find(result.begin(), result.end(), "x=0") != result.end());
    }
    SECTION("mixed signs") {
        auto result = solve("&dom{0..5} = x. &dom{0..5} = y. "
                            "&minimize{ x }. &minimize{ -1*y }.");
        REQUIRE(!result.empty());
        REQUIRE(std::find(result.begin(), result.end(), "x=0 y=5") != result.end());
    }
    SECTION("optimum on the domain bound") { REQUIRE(solve("&dom{5..5} = x. &minimize{ x }.") == S({"x=5"})); }
}

TEST_CASE("minimize-constant", "[solving]") {
    // The minimize constraint is rebuilt for every solve call. The constant
    // of the objective has to keep its sign when that happens.
    Propagator p;
    p.config() = create_configs(0, 10).front();
    SolveEventHandler handler{p};
    Clingo::Control ctl{{"0", "-t1"}};
    ctl.register_propagator(p);
    add_program(ctl, "&dom{0..10} = x. &sum{ x } >= 3. &minimize{ x + 5 }.");
    for (int call = 0; call < 3; ++call) {
        REQUIRE(ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_satisfiable());
        REQUIRE(handler.models.back() == "x=3");
        REQUIRE(ctl.statistics()["user_step"]["Clingcon"]["Cost"].value() == 8);
    }
}

TEST_CASE("priority-multi-shot", "[solving]") {
    SECTION("levels are optimized one after the other") {
        auto models = solve_lex(LEX_PROGRAM);
        REQUIRE(!models.empty());
        REQUIRE(models.back() == "x=37 y=0 z=5");
    }
    SECTION("statistics report the levels") {
        LexReport report;
        auto models = solve_lex(LEX_PROGRAM, &report);
        REQUIRE(!models.empty());
        REQUIRE(report.multi_level);
        REQUIRE(report.levels.has_value());
        REQUIRE(*report.levels == 2);
    }
    SECTION("a level whose seed is already optimal") {
        // Once x is locked at 0, the equality fixes y to 5, so the solve call
        // of the second level finds no model. The third level still has to be
        // optimized: w is maximized.
        auto models = solve_lex("&dom{0..10} = x. &dom{0..10} = y. &dom{0..10} = w. "
                                "&sum{ x; y } = 5. "
                                "&minimize{ x@3 }. &minimize{ y@2 }. &minimize{ -1*w@1 }.");
        REQUIRE(!models.empty());
        REQUIRE(models.back() == "w=10 x=0 y=5");
    }
    SECTION("an unsatisfiable program needs one solve call") {
        LexReport report;
        auto models = solve_lex("&dom{0..5} = x. &dom{0..5} = y. &sum{ x; y } >= 11. "
                                "&minimize{ x@2; y@1 }.",
                                &report);
        REQUIRE(models.empty());
        REQUIRE(report.calls == 1);
    }
    SECTION("a solve call cut short is not stepped") {
        // With a limit of one model the first solve call does not finish
        // its level, so no level is locked and the loop ends.
        auto config = create_configs(0, 100).front();
        config.lex_opt = LexOptMode::MultiShot;
        Propagator p;
        p.config() = config;
        p.enable_steps();
        SolveEventHandler handler{p};
        Clingo::Control ctl{{"1", "-t1"}};
        ctl.register_propagator(p);
        add_program(ctl, LEX_PROGRAM);
        auto result = ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get();
        REQUIRE(!result.is_exhausted());
        REQUIRE(!p.step(result.is_exhausted()));
        REQUIRE(handler.models.size() == 1);
    }
    SECTION("a second step does nothing") {
        auto config = create_configs(0, 100).front();
        config.lex_opt = LexOptMode::MultiShot;
        Propagator p;
        p.config() = config;
        p.enable_steps();
        SolveEventHandler handler{p};
        Clingo::Control ctl{{"0", "-t1"}};
        ctl.register_propagator(p);
        add_program(ctl, LEX_PROGRAM);
        REQUIRE(!p.step(true));
        static_cast<void>(ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get());
        REQUIRE(p.step(true));
        REQUIRE(!p.step(true));
        static_cast<void>(ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get());
        REQUIRE(!p.step(true));
        REQUIRE(handler.models.back() == "x=37 y=0 z=5");
    }
    SECTION("a later program part contradicting a lock is unsatisfiable") {
        auto config = create_configs(0, 10).front();
        config.lex_opt = LexOptMode::MultiShot;
        Propagator p;
        p.config() = config;
        p.enable_steps();
        SolveEventHandler handler{p};
        Clingo::Control ctl{{"0", "-t1"}};
        ctl.register_propagator(p);
        ctl.add("base", {}, THEORY);
        std::string prg = "#program base. &dom { 0..10 } = x. &dom { 0..10 } = y. &sum { x } >= 5. "
                          "&minimize { x@2 }. &minimize { y@1 }. "
                          "#program later. &sum { y } >= 3.";
        Clingo::AST::with_builder(ctl, [&prg](Clingo::AST::ProgramBuilder &builder) -> void {
            Clingo::AST::parse_string(prg.c_str(), [&builder](Clingo::AST::Node const &stm) -> void {
                transform(stm, [&builder](Clingo::AST::Node const &stm) -> void { builder.add(stm); }, true);
            });
        });
        ctl.ground({{"base", {}}});
        bool exhausted = false;
        do {
            exhausted = ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_exhausted();
        } while (p.step(exhausted));
        REQUIRE(handler.models.back() == "x=5 y=0");
        ctl.ground({{"later", {}}});
        REQUIRE(!ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_satisfiable());
    }
    SECTION("a later program part cannot extend a leveled minimize constraint") {
        auto config = create_configs(0, 10).front();
        config.lex_opt = LexOptMode::MultiShot;
        Propagator p;
        p.config() = config;
        p.enable_steps();
        SolveEventHandler handler{p};
        Clingo::Control ctl{{"0", "-t1"}};
        ctl.register_propagator(p);
        ctl.add("base", {}, THEORY);
        std::string prg = "#program base. &dom { 0..10 } = x. &dom { 0..10 } = y. &sum { x } >= 5. "
                          "&minimize { x@2 }. &minimize { y@1 }. "
                          "#program more. &dom { 0..10 } = w. &minimize { w@1 }.";
        Clingo::AST::with_builder(ctl, [&prg](Clingo::AST::ProgramBuilder &builder) -> void {
            Clingo::AST::parse_string(prg.c_str(), [&builder](Clingo::AST::Node const &stm) -> void {
                transform(stm, [&builder](Clingo::AST::Node const &stm) -> void { builder.add(stm); }, true);
            });
        });
        ctl.ground({{"base", {}}});
        bool exhausted = false;
        do {
            exhausted = ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_exhausted();
        } while (p.step(exhausted));
        ctl.ground({{"more", {}}});
        REQUIRE_THROWS(ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get());
    }
    SECTION("a later program part cannot move the minimize constraint to another level") {
        // Without this check the elements of the later part would be merged
        // with the earlier ones as if they were on the same level.
        auto config = create_configs(0, 10).front();
        config.lex_opt = LexOptMode::MultiShot;
        Propagator p;
        p.config() = config;
        p.enable_steps();
        SolveEventHandler handler{p};
        Clingo::Control ctl{{"0", "-t1"}};
        ctl.register_propagator(p);
        ctl.add("base", {}, THEORY);
        std::string prg = "#program base. &dom { 0..3 } = x. &dom { 0..3 } = y. &sum { x; y } >= 3. "
                          "&minimize { x@1 }. "
                          "#program later. &minimize { 2*y@0 }.";
        Clingo::AST::with_builder(ctl, [&prg](Clingo::AST::ProgramBuilder &builder) -> void {
            Clingo::AST::parse_string(prg.c_str(), [&builder](Clingo::AST::Node const &stm) -> void {
                transform(stm, [&builder](Clingo::AST::Node const &stm) -> void { builder.add(stm); }, true);
            });
        });
        ctl.ground({{"base", {}}});
        bool exhausted = false;
        do {
            exhausted = ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_exhausted();
        } while (p.step(exhausted));
        REQUIRE(handler.models.back() == "x=0 y=3");
        ctl.ground({{"later", {}}});
        REQUIRE_THROWS(ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get());
    }
    SECTION("a later program part extends the minimize constraint on its level") {
        auto config = create_configs(0, 10).front();
        config.lex_opt = LexOptMode::MultiShot;
        Propagator p;
        p.config() = config;
        p.enable_steps();
        SolveEventHandler handler{p};
        Clingo::Control ctl{{"0", "-t1"}};
        ctl.register_propagator(p);
        ctl.add("base", {}, THEORY);
        std::string prg = "#program base. &dom { 0..3 } = x. &dom { 0..3 } = y. &sum { x; y } >= 3. "
                          "&minimize { x@1 }. "
                          "#program later. &minimize { 2*y@1 }.";
        Clingo::AST::with_builder(ctl, [&prg](Clingo::AST::ProgramBuilder &builder) -> void {
            Clingo::AST::parse_string(prg.c_str(), [&builder](Clingo::AST::Node const &stm) -> void {
                transform(stm, [&builder](Clingo::AST::Node const &stm) -> void { builder.add(stm); }, true);
            });
        });
        ctl.ground({{"base", {}}});
        bool exhausted = false;
        do {
            exhausted = ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_exhausted();
        } while (p.step(exhausted));
        REQUIRE(handler.models.back() == "x=0 y=3");
        ctl.ground({{"later", {}}});
        do {
            exhausted = ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_exhausted();
        } while (p.step(exhausted));
        REQUIRE(handler.models.back() == "x=3 y=0");
    }
    SECTION("a level whose optimum is not a value cannot be fixed") {
        // The first level is proven at 2000000000 and the second one needs a
        // solve call, so the first one has to be fixed.
        REQUIRE_THROWS(solve_lex("&dom{0..3} = x. &dom{0..1} = y. &sum{ x; y } >= 3. "
                                 "&minimize{ 1000000000*x@2 }. &minimize{ y@1 }."));
    }
    SECTION("one level is no level") {
        LexReport report;
        auto models = solve_lex("&dom{0..5} = x. &dom{0..5} = y. &sum{ x; y } >= 5. &minimize{ x@1; y@1 }.", &report);
        REQUIRE(!models.empty());
        REQUIRE(!report.multi_level);
        REQUIRE(report.calls == 1);
    }
    SECTION("multi-shot without steps is rejected") {
        auto config = create_configs(0, 100).front();
        config.lex_opt = LexOptMode::MultiShot;
        Propagator p;
        p.config() = config;
        SolveEventHandler handler{p};
        Clingo::Control ctl{{"0", "-t1"}};
        ctl.register_propagator(p);
        add_program(ctl, SMALL_PROGRAM);
        REQUIRE_THROWS(ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get());
    }
    SECTION("a missing step is reported") {
        CaptureCerr capture;
        {
            auto config = create_configs(0, 100).front();
            config.lex_opt = LexOptMode::MultiShot;
            Propagator p;
            p.config() = config;
            p.enable_steps();
            SolveEventHandler handler{p};
            Clingo::Control ctl{{"0", "-t1"}};
            ctl.register_propagator(p);
            add_program(ctl, SMALL_PROGRAM);
            static_cast<void>(ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get());
        }
        REQUIRE(capture.str().find("clingcon_step") != std::string::npos);
    }
}

TEST_CASE("priority-weighted", "[solving]") {
    SECTION("several threads fold the levels") {
        // The harness solves with eight threads, so auto picks weighted.
        REQUIRE(solve(SMALL_PROGRAM) == S({"x=0 y=5"}));
    }
    SECTION("levels that do not fit into the cost are rejected") {
        auto config = create_configs(0, MAX_VAL).front();
        config.lex_opt = LexOptMode::Weighted;
        // the weight of the fourth level exceeds the range of the cost
        REQUIRE_THROWS(solve_lex(config, "&dom{0..1000000000} = a. &dom{0..1000000000} = b. "
                                         "&dom{0..1000000000} = c. &dom{0..1000000000} = d. "
                                         "&minimize{ a@4; b@3; c@2; d@1 }."));
        // the weights fit, the folded objective does not
        REQUIRE_THROWS(solve_lex(config, "&dom{0..1000000000} = a. &dom{0..1000000000} = b. "
                                         "&dom{0..1000000000} = c. "
                                         "&minimize{ a@3; b@2; c@1 }."));
    }
    SECTION("weights beyond the range of a value") {
        // The weight of the second level is above 2^30, so the folded
        // constraint has a coefficient that is not a value.
        auto config = create_configs(0, MAX_VAL).front();
        config.lex_opt = LexOptMode::Weighted;
        auto models = solve_lex(config, "&dom{0..3} = a. &dom{0..1} = b. &sum{ a; b } >= 1. "
                                        "&minimize{ 1000000000*a@1 }. &minimize{ b@2 }.");
        REQUIRE(!models.empty());
        REQUIRE(models.back() == "a=1 b=0");
        // Three levels over domains whose product exceeds 2^30; the sum
        // constraint pins the optimum, so the search is short.
        models = solve_lex(config, "&dom{0..40000} = x. &dom{0..40000} = y. &dom{0..40000} = z. "
                                   "&sum{ x; y; z } >= 120000. "
                                   "&minimize{ x@3 }. &minimize{ y@2 }. &minimize{ z@1 }.");
        REQUIRE(!models.empty());
        REQUIRE(models.back() == "x=40000 y=40000 z=40000");
        // The same with the optimum below the domain bounds.
        models = solve_lex(config, "&dom{0..40000} = x. &dom{0..40000} = y. &dom{0..40000} = z. "
                                   "&sum{ x; y; z } >= 119998. &sum{ y } <= 39999. "
                                   "&minimize{ x@3 }. &minimize{ y@2 }. &minimize{ z@1 }.");
        REQUIRE(!models.empty());
        REQUIRE(models.back() == "x=39999 y=39999 z=40000");
    }
    SECTION("a variable on several levels") {
        // x has opposite signs on the two levels; the folded constraint has
        // one element for it.
        auto config = create_configs(0, 100).front();
        config.lex_opt = LexOptMode::Weighted;
        auto models = solve_lex(config, "&dom{0..3} = x. &dom{0..3} = z. &sum{ 2*x; z } != 3. "
                                        "&minimize{ 2*x@2; z@2 }. &minimize{ z@1; -2*x@1 }.");
        REQUIRE(!models.empty());
        REQUIRE(models.back() == "x=0 z=0");
    }
    SECTION("weighted overrides the choice of auto") {
        auto config = create_configs(0, 100).front();
        config.lex_opt = LexOptMode::Weighted;
        LexReport report;
        auto models = solve_lex(config, SMALL_PROGRAM, &report);
        REQUIRE(!report.multi_level);
        REQUIRE(!models.empty());
        REQUIRE(models.back() == "x=0 y=5");
    }
}
