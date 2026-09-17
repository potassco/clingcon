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
// order they are found; the last one is the lexicographic optimum.
auto solve_lex(Config const &config, std::string const &prg) -> S {
    Propagator p;
    p.config() = config;
    p.enable_steps();
    SolveEventHandler handler{p};
    Clingo::Control ctl{{"0", "-t1"}};
    ctl.register_propagator(p);
    add_program(ctl, prg);

    bool exhausted = false;
    do {
        auto result = ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get();
        if (result.is_interrupted()) {
            throw std::runtime_error("interrupted");
        }
        exhausted = result.is_exhausted();
    } while (p.step(exhausted));

    return handler.models;
}

auto solve_lex(std::string const &prg) -> S {
    auto config = create_configs(0, 100).front();
    config.lex_opt = LexOptMode::MultiShot;
    return solve_lex(config, prg);
}

// Two levels whose lexicographic optimum is reached only after the first one
// is locked: z >= 42-x pushes x to 37 once x+y is fixed at its optimum.
constexpr char const *LEX_PROGRAM = "&dom { 0..50 } = x. &dom { 0..50 } = y. &dom { 0..50 } = z. "
                                    "&sum { x; y } >= 37. &sum { z; x } >= 42. "
                                    "&minimize { x@2; y@2 }. &minimize { z@1 }.";

// Two tasks on one machine and the makespan m. The first task takes three
// time units and precedes the second one, which takes two. The optimum is
// s1=0, s2=3, m=5.
constexpr char const *SCHEDULE = "&dom{0..10} = s1. &dom{0..10} = s2. &dom{0..10} = m. "
                                 "&sum{ s2; -1*s1 } >= 3. "
                                 "&sum{ m; -1*s1 } >= 3. &sum{ m; -1*s2 } >= 2. "
                                 "&minimize{ m }.";

} // namespace

TEST_CASE("heuristic-parsing", "[parsing]") {
    SECTION("the priority has to be non-negative") {
        REQUIRE_THROWS(solve("&dom{1..3} = x. &heuristic(-1, input_order, indomain_min) { x }."));
    }
    SECTION("the optimization level can be negative like a minimize level") {
        REQUIRE(!solve("&dom{1..3} = x. &heuristic(0, input_order, indomain_min, -1) { x }.").empty());
        REQUIRE(!solve("&dom{1..3} = x. &heuristic(0, input_order, indomain_min, none, -1) { x }.").empty());
        REQUIRE_THROWS(solve("&dom{1..3} = x. &heuristic(0, input_order, indomain_min, none, x) { x }."));
    }
    SECTION("unknown strategies") {
        REQUIRE_THROWS(solve("&dom{1..3} = x. &heuristic(0, most_constrained, indomain_min) { x }."));
        REQUIRE_THROWS(solve("&dom{1..3} = x. &heuristic(0, input_order, indomain_median) { x }."));
        REQUIRE_THROWS(solve("&dom{1..3} = x. &heuristic(0, first_fail, indomain_min, width) { x }."));
    }
    SECTION("a number where the tie-breaker and the level are swapped") {
        REQUIRE_THROWS(solve("&dom{1..3} = x. &heuristic(0, first_fail, indomain_min, 1, degree) { x }."));
    }
    SECTION("the elements are variables") {
        REQUIRE_THROWS(solve("&dom{1..3} = x. &heuristic(0, input_order, indomain_min) { 1 }."));
    }
}

TEST_CASE("heuristic-annotations", "[solving]") {
    SECTION("a directive does not restrict the models") {
        REQUIRE(solve("&dom{1..3} = x. &dom{1..3} = y. "
                      "&heuristic(0, input_order, indomain_min) { x; y }.")
                    .size() == 9);
    }
    SECTION("first_fail takes the smallest domain") {
        REQUIRE(solve("&dom{1..2} = x. &dom{1..5} = y. "
                      "&heuristic(0, first_fail, indomain_min) { x; y }.")
                    .size() == 10);
    }
    SECTION("degree takes the variable most constraints watch") {
        // Two constraints watch x, one watches y.
        REQUIRE(solve("&dom{1..5} = x. &dom{1..5} = y. "
                      "&sum{ x } <= 4. &sum{ x } >= 2. &sum{ y } <= 4. "
                      "&heuristic(0, degree, indomain_min) { x; y }.")
                    .size() == 12);
    }
    SECTION("degree with a variable no constraint watches") {
        REQUIRE(solve("&dom{1..3} = x. &dom{1..3} = y. &dom{1..3} = z. "
                      "&sum{ x } >= 2. &sum{ y } >= 2. "
                      "&heuristic(0, degree, indomain_min) { x; y; z }.")
                    .size() == 12);
    }
    SECTION("dom_deg compares domain width to degree") {
        // The ratio of x is 7/2, the one of y is 3/1.
        REQUIRE(solve("&dom{1..10} = x. &dom{1..4} = y. "
                      "&sum{ x } <= 8. &sum{ x } >= 2. &sum{ y } <= 3. "
                      "&heuristic(0, dom_deg, indomain_min) { x; y }.")
                    .size() == 21);
    }
    SECTION("dom_deg with a variable no constraint watches") {
        REQUIRE(solve("&dom{1..3} = x. &dom{1..3} = y. &dom{1..3} = z. "
                      "&sum{ x } >= 2. "
                      "&heuristic(0, dom_deg, indomain_min) { x; y; z }.")
                    .size() == 18);
    }
    SECTION("first_fail with the degree tie-breaker") {
        // Both domains have width two; two constraints watch x, one watches y.
        REQUIRE(solve("&dom{1..4} = x. &dom{1..4} = y. "
                      "&sum{ x } <= 3. &sum{ x } >= 2. &sum{ y } <= 3. "
                      "&heuristic(0, first_fail, indomain_min, degree) { x; y }.")
                    .size() == 6);
    }
    SECTION("an empty set covers all integer variables") {
        REQUIRE(solve("&dom{1..3} = x. &dom{1..3} = y. "
                      "&heuristic(0, first_fail, indomain_min) { }.")
                    .size() == 9);
    }
    SECTION("an omitted set covers all integer variables") {
        REQUIRE(solve("&dom{1..3} = x. &dom{1..3} = y. "
                      "&heuristic(0, input_order, indomain_min).")
                    .size() == 9);
    }
    SECTION("a directive scoped to an optimization level") {
        auto result = solve("&dom{0..5} = x. &dom{0..5} = y. "
                            "&minimize{ x @2 }. &minimize{ y @1 }. "
                            "&heuristic(0, smallest, indomain_min, 2) { x }. "
                            "&heuristic(0, first_fail, indomain_min, degree, 1) { y }.");
        REQUIRE(std::find(result.begin(), result.end(), "x=0 y=0") != result.end());
    }
}

TEST_CASE("heuristic-tie-breaker-warning", "[parsing]") {
    SECTION("a tie-breaker without effect is reported") {
        CaptureCerr capture;
        REQUIRE(solve("&dom{1..3} = x. &dom{1..3} = y. "
                      "&heuristic(0, dom_deg, indomain_min, degree) { x; y }.")
                    .size() == 9);
        REQUIRE(capture.str().find("tie-breaker 'degree' has no effect") != std::string::npos);
        REQUIRE(capture.str().find("'dom_deg'") != std::string::npos);
    }
    SECTION("no report for a variable selection that can tie") {
        CaptureCerr capture;
        REQUIRE(solve("&dom{1..3} = x. &dom{1..3} = y. "
                      "&heuristic(0, first_fail, indomain_min, degree) { x; y }.")
                    .size() == 9);
        REQUIRE(capture.str().find("tie-breaker") == std::string::npos);
    }
}

TEST_CASE("heuristic-var-selection", "[solving]") {
    SECTION("smallest takes the smallest lower bound") {
        REQUIRE(solve("&dom{5..10} = x. &dom{1..10} = y. "
                      "&heuristic(0, smallest, indomain_min) { x; y }.")
                    .size() == 60);
    }
    SECTION("largest takes the largest upper bound") {
        REQUIRE(solve("&dom{1..3} = x. &dom{1..3} = y. "
                      "&heuristic(0, largest, indomain_max) { x; y }.")
                    .size() == 9);
    }
    SECTION("anti_first_fail takes the largest domain") {
        REQUIRE(solve("&dom{1..2} = x. &dom{1..10} = y. "
                      "&heuristic(0, anti_first_fail, indomain_min) { x; y }.")
                    .size() == 20);
    }
    SECTION("a directive whose variables are all assigned") {
        REQUIRE(solve("&dom{5..5} = x. &dom{3..3} = y. "
                      "&heuristic(0, first_fail, indomain_min) { x; y }.") == S({"x=5 y=3"}));
    }
    SECTION("two priorities") {
        REQUIRE(solve("&dom{1..3} = x. &dom{1..3} = y. &dom{1..3} = z. "
                      "&heuristic(0, first_fail, indomain_min) { x }. "
                      "&heuristic(1, largest, indomain_max) { y; z }.")
                    .size() == 27);
    }
}

TEST_CASE("heuristic-val-selection", "[solving]") {
    SECTION("indomain_min") {
        REQUIRE(solve("&dom{1..5} = x. &sum{ x } >= 3. "
                      "&heuristic(0, input_order, indomain_min) { x }.")
                    .size() == 3);
    }
    SECTION("indomain_max") {
        REQUIRE(solve("&dom{1..5} = x. &sum{ x } <= 3. "
                      "&heuristic(0, input_order, indomain_max) { x }.")
                    .size() == 3);
    }
    SECTION("indomain_split") {
        REQUIRE(solve("&dom{1..8} = x. &heuristic(0, input_order, indomain_split) { x }.").size() == 8);
    }
    SECTION("indomain_reverse_split") {
        REQUIRE(solve("&dom{1..8} = x. &heuristic(0, input_order, indomain_reverse_split) { x }.").size() == 8);
    }
    SECTION("a domain with one value") {
        REQUIRE(solve("&dom{5..5} = x. &heuristic(0, input_order, indomain_min) { x }.") == S({"x=5"}));
    }
}

TEST_CASE("heuristic-vsids", "[solving]") {
    SECTION("the models are the same as without the directive") {
        auto with = solve("&dom{1..4} = x. &dom{1..4} = y. "
                          "&heuristic(0, vsids, indomain_min) { x; y }.");
        REQUIRE(with == solve("&dom{1..4} = x. &dom{1..4} = y."));
    }
    SECTION("vsids over all integer variables") {
        REQUIRE(solve("&dom{1..3} = x. &dom{1..3} = y. &heuristic(0, vsids, indomain_min) { }.").size() == 9);
    }
    SECTION("vsids with a minimize directive") {
        auto result = solve("&dom{0..10} = x. &dom{0..10} = y. &sum{ x; y } >= 5. "
                            "&minimize{ x }. "
                            "&heuristic(0, vsids, indomain_min) { x; y }.");
        REQUIRE(std::find(result.begin(), result.end(), "x=0 y=5") != result.end());
    }
    SECTION("vsids with a value selection that splits") {
        REQUIRE(solve("&dom{0..10} = x. &dom{0..10} = y. &sum{ x; y } = 10. "
                      "&heuristic(0, vsids, indomain_split) { x; y }.")
                    .size() == 11);
    }
}

TEST_CASE("heuristic-levels", "[solving]") {
    SECTION("a directive acts on the level it is scoped to") {
        // The directive pushes z to its upper bound 50. Scoped to the level
        // of z it acts after the level of x and y is locked, so the first
        // model of the first solve call keeps z low. Scoped to the level of x
        // and y it acts in the first solve call instead. Either way the
        // lexicographic optimum is x=37 y=0 z=5.
        auto lower = solve_lex(std::string{LEX_PROGRAM} + " &heuristic(0, input_order, indomain_max, 1) { z }.");
        auto higher = solve_lex(std::string{LEX_PROGRAM} + " &heuristic(0, input_order, indomain_max, 2) { z }.");
        REQUIRE(lower.front().find("z=50") == std::string::npos);
        REQUIRE(higher.front().find("z=50") != std::string::npos);
        REQUIRE(lower.back() == "x=37 y=0 z=5");
        REQUIRE(higher.back() == "x=37 y=0 z=5");
    }
    SECTION("an unscoped directive acts on every level") {
        auto models = solve_lex(std::string{LEX_PROGRAM} + " &heuristic(0, input_order, indomain_max) { z }.");
        REQUIRE(models.front().find("z=50") != std::string::npos);
        REQUIRE(models.back() == "x=37 y=0 z=5");
    }
    SECTION("a scoped directive is reported in weighted mode") {
        std::string prg = "&dom{0..5} = x. &dom{0..5} = y. &sum{ x; y } >= 5. "
                          "&minimize{ x@2; y@1 }. "
                          "&heuristic(0, input_order, indomain_min, 1) { y }.";
        auto config = create_configs(0, 100).front();
        config.lex_opt = LexOptMode::Weighted;
        CaptureCerr capture;
        REQUIRE(solve_lex(config, prg).back() == "x=0 y=5");
        REQUIRE(capture.str().find("do not act in weighted mode") != std::string::npos);
    }
    SECTION("nothing is reported in multi-shot mode") {
        std::string prg = "&dom{0..5} = x. &dom{0..5} = y. &sum{ x; y } >= 5. "
                          "&minimize{ x@2; y@1 }. "
                          "&heuristic(0, input_order, indomain_min, 1) { y }.";
        CaptureCerr capture;
        REQUIRE(solve_lex(prg).back() == "x=0 y=5");
        REQUIRE(capture.str().empty());
    }
}

TEST_CASE("order-heuristic", "[solving]") {
    auto expected = solve(SCHEDULE);
    REQUIRE(expected == S({"m=5 s1=0 s2=3"}));
    SECTION("earliest-start") {
        auto config = create_configs().front();
        config.default_solver_config.heuristic = Heuristic::EarliestStart;
        REQUIRE(solve(config, SCHEDULE) == expected);
    }
    SECTION("tightest-window") {
        auto config = create_configs().front();
        config.default_solver_config.heuristic = Heuristic::TightestWindow;
        REQUIRE(solve(config, SCHEDULE) == expected);
    }
}

namespace {

// Runs the solve calls of a leveled program and returns the first model of
// each call.
auto first_models(std::string const &prg, Clingo::PartSpan const &parts = {{"base", {}}}) -> S {
    auto config = create_configs(0, 50).front();
    config.lex_opt = LexOptMode::MultiShot;
    Propagator p;
    p.config() = config;
    p.enable_steps();
    SolveEventHandler handler{p};
    Clingo::Control ctl{{"0", "-t1"}};
    ctl.register_propagator(p);
    ctl.add("base", {}, THEORY);
    Clingo::AST::with_builder(ctl, [&prg](Clingo::AST::ProgramBuilder &builder) -> void {
        Clingo::AST::parse_string(prg.c_str(), [&builder](Clingo::AST::Node const &stm) -> void {
            transform(stm, [&builder](Clingo::AST::Node const &stm) -> void { builder.add(stm); }, true);
        });
    });
    S firsts;
    for (auto const &part : parts) {
        ctl.ground({part});
        bool exhausted = false;
        do {
            auto before = handler.models.size();
            exhausted = ctl.solve(Clingo::LiteralSpan{}, &handler, false, false).get().is_exhausted();
            if (handler.models.size() > before) {
                firsts.emplace_back(handler.models[before]);
            }
        } while (p.step(exhausted));
    }
    return firsts;
}

} // namespace

TEST_CASE("heuristic-solve-calls", "[solving]") {
    // Three levels whose optima are not visible on decision level zero, so
    // that every level gets a solve call, and a free variable f that only a
    // directive pushes to its upper bound.
    constexpr char const *LEVELS = "&dom { 0..50 } = x. &dom { 0..50 } = y. &dom { 0..50 } = z. &dom { 0..50 } = f. "
                                   "{ a }. &sum { x } >= 20 :- a. &sum { x } >= 10 :- not a. "
                                   "{ b }. &sum { y } >= 20 :- b. &sum { y } >= 10 :- not b. "
                                   "{ c }. &sum { z } >= 20 :- c. &sum { z } >= 10 :- not c. "
                                   "&minimize { x@3 }. &minimize { y@2 }. &minimize { z@1 }. ";
    SECTION("a directive scoped to the middle level acts in the second solve call") {
        auto firsts = first_models(std::string{LEVELS} + "&heuristic(0, input_order, indomain_max, 2) { f }.");
        REQUIRE(firsts.size() == 3);
        REQUIRE(firsts[0].find("f=0") != std::string::npos);
        REQUIRE(firsts[1].find("f=50") != std::string::npos);
    }
    SECTION("a directive scoped to the last level acts in the last solve call") {
        auto firsts = first_models(std::string{LEVELS} + "&heuristic(0, input_order, indomain_max, 1) { f }.");
        REQUIRE(firsts.size() == 3);
        REQUIRE(firsts[0].find("f=0") != std::string::npos);
        REQUIRE(firsts[2].find("f=50") != std::string::npos);
    }
    SECTION("a directive over all variables covers a later program part") {
        auto firsts = first_models("#program base. &dom { 0..50 } = f. &heuristic(0, input_order, indomain_max) { }. "
                                   "#program later. &dom { 0..50 } = g.",
                                   {{"base", {}}, {"later", {}}});
        REQUIRE(firsts.size() == 2);
        REQUIRE(firsts[0].find("f=50") != std::string::npos);
        REQUIRE(firsts[1].find("g=50") != std::string::npos);
    }
}
