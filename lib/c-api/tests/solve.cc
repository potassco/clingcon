// {{{ MIT License
//
// Copyright Roland Kaminski, Philipp Wanko, and Max Ostrowski
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

#include <clingcon.h>
#include <clingcon/propagator.hh>

#include <clingo/theory.hh>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <iostream>

namespace Clingcon {

using namespace std::string_view_literals;

namespace {

struct Fixture {
    //! A DL assignment.
    using A = std::pair<Clingo::Symbol, double>;
    //! A vector of DL assignments.
    using AV = std::vector<A>;
    //! A vector of symbols.
    using SV = std::vector<Clingo::Symbol>;
    //! A solution in form of a pair of DL assignments and symbols.
    using SP = std::pair<AV, SV>;
    //! A vector solutions.
    using RV = std::vector<SP>;

    //! Encoding for the job shop problem.
    static constexpr char const *ENC = R"(
#const bound=16.

&minimize { bound }.

            machine(1).      machine(2).
task(a). duration(a,1,3). duration(a,2,4).
task(b). duration(b,1,1). duration(b,2,6).
task(c). duration(c,1,5). duration(c,2,5).

1 { cycle(T,U) : task(U), U != T } 1 :- task(T).
1 { cycle(T,U) : task(T), U != T } 1 :- task(U).

reach(M) :- M = #min { T : task(T) }.
reach(U) :- reach(T), cycle(T,U).
:- task(T), not reach(T).

1 { start(T) : task(T) } 1.

permutation(T,U) :- cycle(T,U), not start(U).

seq((T,M),(T,M+1),D) :- task(T), duration(T,M,D), machine(M+1).
seq((T1,M),(T2,M),D) :- permutation(T1,T2), duration(T1,M,D).

&sum {  1*T1 + -1*T2 } <= -D :- seq(T1,T2,D).
&sum { -1*(T,M) } <= 0       :- duration(T,M,D).
&sum {  1*(T,M) } <= bound-D :- duration(T,M,D).

#show permutation/2.
)";

    Fixture() { theory.register_theory(ctl); }

    //! Create a symbol for sequence atoms of task/machine pairs.
    [[nodiscard]] auto seq(Clingo::Symbol const &a, Clingo::Symbol const &b, Clingo::Symbol const &c) const
        -> std::vector<Clingo::Symbol> {
        auto x = Clingo::Function(lib, "permutation", {a, b});
        auto y = Clingo::Function(lib, "permutation", {b, c});
        if (x < y) {
            return {x, y};
        }
        return {y, x};
    }

    //! A DL assignment for task/machine pairs.
    [[nodiscard]] auto ass(Clingo::Symbol const &a, int b, int c) const -> A {
        return A(Clingo::Tuple(lib, {a, Clingo::Number(b)}), c);
    }

    [[nodiscard]] auto ass(int a1, int a2, int b1, int b2, int c1, int c2) const -> std::vector<A> {
        auto a = Clingo::Function(lib, "a", {});
        auto b = Clingo::Function(lib, "b", {});
        auto c = Clingo::Function(lib, "c", {});
        return {
            ass(a, 1, a1), ass(a, 2, a2), ass(b, 1, b1), ass(b, 2, b2), ass(c, 1, c1), ass(c, 2, c2),
        };
    }

    [[nodiscard]] auto sols() const -> RV {
        auto a = Clingo::Function(lib, "a", {});
        auto b = Clingo::Function(lib, "b", {});
        auto c = Clingo::Function(lib, "c", {});
        return {
            SP{ass(1, 7, 0, 1, 4, 11), {seq(b, a, c)}}, // NOLINT
            SP{ass(1, 7, 0, 1, 5, 11), {seq(b, a, c)}}, // NOLINT
            SP{ass(1, 7, 0, 1, 6, 11), {seq(b, a, c)}}, // NOLINT
            SP{ass(2, 7, 0, 1, 5, 11), {seq(b, a, c)}}, // NOLINT
            SP{ass(2, 7, 0, 1, 6, 11), {seq(b, a, c)}}, // NOLINT
            SP{ass(3, 7, 0, 1, 6, 11), {seq(b, a, c)}}, // NOLINT
            SP{ass(6, 12, 0, 1, 1, 7), {seq(b, c, a)}}, // NOLINT
            SP{ass(7, 12, 0, 1, 1, 7), {seq(b, c, a)}}, // NOLINT
            SP{ass(7, 12, 0, 1, 2, 7), {seq(b, c, a)}}, // NOLINT
            SP{ass(8, 12, 0, 1, 1, 7), {seq(b, c, a)}}, // NOLINT
            SP{ass(8, 12, 0, 1, 2, 7), {seq(b, c, a)}}, // NOLINT
            SP{ass(9, 12, 0, 1, 1, 7), {seq(b, c, a)}}, // NOLINT
            SP{ass(9, 12, 0, 1, 2, 7), {seq(b, c, a)}}, // NOLINT
        };
    }
    //! Solutions to the task assignment problem.
    //! A handler to gather statistics in a DL theory.
    class Handler : public Clingo::SolveEventHandler {
      public:
        Handler(Clingo::Theory &theory) : theory_{&theory} {}
        //! Add theory specific statistics.
        void do_stats(Clingo::Stats step, Clingo::Stats accu) override { theory_->stats(step, accu); }

      private:
        Clingo::Theory *theory_; //!< The DL theory.
    };

    //! Solve a given problem returning all models.
    auto solve(Clingo::Control &ctl) -> RV {
        using namespace Clingo;
        RV result;
        for (auto &&m : ctl.start_solve({}, SolveFlags::yield, Handler{theory})) {
            result.emplace_back();
            auto &sol = result.back().first;
            auto &sol_bool = result.back().second;
            for (auto &[key, value] : theory.assignment(m.thread_id())) {
                if (auto *num = std::get_if<int>(&value)) {
                    sol.emplace_back(key, *num);
                } else if (auto *num = std::get_if<double>(&value)) {
                    sol.emplace_back(key, *num);
                } else {
                    REQUIRE(false);
                }
            }
            std::ranges::sort(sol);
            for (auto const &s : m.symbols()) {
                sol_bool.emplace_back(s);
            }
            std::ranges::sort(sol_bool);
        }
        std::ranges::sort(result);
        return result;
    }

    [[maybe_unused]] static void print(RV const &result) {
        for (auto const &[ass, syms] : result) {
            std::cerr << "solution:";
            std::cerr << "\n  symbols:";
            for (auto const &sym : syms) {
                std::cerr << " " << sym;
            }
            std::cerr << "\n  assignment:";
            for (auto const &[sym, val] : ass) {
                std::cerr << " " << sym << "=" << val;
            }
            std::cerr << "\n";
            std::cerr.flush();
        }
    }

    Clingo::Library lib;
    Clingo::Theory theory{lib, clingcon_create};
    Clingo::Control ctl{lib, {"0"}};
    Clingo::Config cfg{ctl.config()};
    Clingo::Symbol sym_a = Function(lib, "a");
    Clingo::Symbol sym_b = Function(lib, "b");
    Clingo::Symbol sym_c = Function(lib, "c");
    Clingo::Symbol sym_d = Function(lib, "d");
    Clingo::Symbol sym_e = Function(lib, "e");
    Clingo::Symbol sym_f = Tuple(lib, {Function(lib, "f"), Function(lib, "f")});
};

} // namespace

TEST_CASE_METHOD(Fixture, "solving base", "[clingo]") { // NOLINT
    theory.rewrite(lib, ctl,
                   "1 { a; b } 1. &diff { a - b } = 3.\n"
                   "&diff { 0 - a } = -5 :- a.\n"
                   "&diff { 0 - b } = -7 :- b.\n");
    ctl.ground();
    theory.prepare(ctl);
    auto result = solve(ctl);
    REQUIRE(result == (RV{{{{sym_a, 5}, {sym_b, 2}}, {sym_a}}, {{{sym_a, 10}, {sym_b, 7}}, {sym_b}}}));

    theory.rewrite(lib, ctl,
                   "#program ext.\n"
                   "&diff { a - 0 } <= 6.\n");
    ctl.ground({{"ext", {}}});
    theory.prepare(ctl);
    result = solve(ctl);
    REQUIRE(result == (RV{{{{sym_a, 5}, {sym_b, 2}}, {sym_a}}}));
}

TEST_CASE_METHOD(Fixture, "solving not_equal", "[clingo]") {
    theory.rewrite(lib, ctl, "&dom { 5..6 } = b. { a }. &diff { b - 0 } != 5 :- not a.\n");
    ctl.ground();
    theory.prepare(ctl);
    auto result = solve(ctl);
    REQUIRE(result == (RV{{{{sym_b, 5}}, {sym_a}}, {{{sym_b, 6}}, {}}, {{{sym_b, 6}}, {sym_a}}}));
}

TEST_CASE_METHOD(Fixture, "solving configure", "[clingo]") {
    cfg["clingcon.shift_constraints"] = "yes";
    theory.rewrite(lib, ctl, " :- &sum { a } != 5.\n");
    ctl.ground();
    theory.prepare(ctl);
    auto result = solve(ctl);
    REQUIRE(result == (RV{{{{sym_a, 5}}, {}}}));
}

TEST_CASE_METHOD(Fixture, "solving normalize", "[clingo]") {
    theory.rewrite(lib, ctl,
                   "&sum { a } >= 6.\n"
                   "&sum { a } = b.\n"
                   "&sum { 5 } >= 0.\n"
                   "&sum { c } > b.\n"
                   "&sum { c } <= d + 1.\n"
                   "&sum { d } <= 6.\n"
                   "&sum { e } != (f,f).\n"
                   "&sum { e } = 5.\n"
                   "&dom { 5..6 } = (f,f).\n");
    ctl.ground();
    theory.prepare(ctl);
    auto result = solve(ctl);
    REQUIRE(result == RV{{{{sym_f, 6}, {sym_a, 6}, {sym_b, 6}, {sym_c, 7}, {sym_d, 6}, {sym_e, 5}}, {}}});
}

TEST_CASE_METHOD(Fixture, "solving empty", "[clingo]") {
    theory.rewrite(lib, ctl,
                   "{ b }.\n"
                   "&sum { 0 } < -4 :- b.\n");
    ctl.ground();
    theory.prepare(ctl);
    auto result = solve(ctl);
    REQUIRE(result == (RV{{{}, {}}}));
    REQUIRE(ctl.stats()["solving"]["solvers"]["choices"].value() == 0);
}

TEST_CASE_METHOD(Fixture, "solving symbols", "[clingo]") {
    theory.rewrite(lib, ctl, "&diff{ (\"foo\\\\\\nbar\\\"foo\",123) - 0 } = 17.\n");
    ctl.ground();
    theory.prepare(ctl);
    auto result = solve(ctl);
    REQUIRE(result == (RV{{{{Tuple(lib, {String(lib, "foo\\\nbar\"foo"), Clingo::Number(123)}), 17}}, {}}}));
}

TEST_CASE_METHOD(Fixture, "solving task-assignment", "[clingo]") {
    const auto *on_off = GENERATE("0", "1");
    cfg["clingcon.translate_clauses"] = GENERATE("1", "1000");
    cfg["clingcon.sign_value"] = GENERATE("0", "+", "-");
    cfg["clingcon.shift_constraints"] = on_off;
    cfg["clingcon.sort_constraints"] = on_off;
    cfg["clingcon.literals_only"] = on_off;
    cfg["clingcon.translate_pb"] = "1.5";
    cfg["clingcon.translate_distinct"] = "2";
    cfg["clingcon.translate_opt"] = "0";
    cfg["clingcon.add_order_clauses"] = on_off;
    cfg["clingcon.min_int"] = "-1000";
    cfg["clingcon.max_int"] = "1000";
    cfg["clingcon.check_solution"] = "1";
    cfg["clingcon.check_state"] = "1";
    cfg["clingcon.order_heuristic"] = "max-chain";
    cfg["clingcon.refine_reasons"] = on_off;
    cfg["clingcon.refine_introduce"] = on_off;
    cfg["clingcon.propagate_chain"] = "1";
    cfg["clingcon.split_all"] = on_off;
    theory.rewrite(lib, ctl, ENC);
    ctl.ground();
    theory.prepare(ctl);
    auto result = solve(ctl);
    REQUIRE(solve(ctl) == sols());
}

} // namespace Clingcon
