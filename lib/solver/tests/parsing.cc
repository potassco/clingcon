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

#include "clingcon/parsing.hh"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>

#include <clingo/ast.hh>
#include <clingo/control.hh>
#include <map>
#include <sstream>
#include <string_view>

namespace Clingcon {

namespace {

class TestBuilder : public Clingcon::AbstractConstraintBuilder {
  public:
    TestBuilder(std::ostringstream &oss) : oss_{oss} {}
    TestBuilder(TestBuilder const &) = delete;
    TestBuilder(TestBuilder &&) = delete;
    auto operator=(TestBuilder const &) -> TestBuilder & = delete;
    auto operator=(TestBuilder &&) -> TestBuilder & = delete;
    ~TestBuilder() override = default;

    auto solver_literal(lit_t literal) -> lit_t override { return literal == 0 ? 1 : 2; }

    auto is_true(lit_t literal) -> bool override { return literal == 1; }

    auto value(lit_t literal) -> std::optional<bool> override {
        if (literal == 1) {
            return true;
        }
        if (literal == -1) {
            return false;
        }
        return std::nullopt;
    }

    auto add_literal() -> lit_t override { return ++literals_; }

    auto add_clause(Clingo::SolverLiteralSpan clause) -> bool override {
        bool sep{false};
        oss_ << "{ ";
        for (auto const &lit : clause) {
            oss_ << (sep ? ", " : "") << lit;
            sep = true;
        }
        oss_ << " }.";
        return true;
    }

    void add_show() override {
        if (!show_) {
            oss_ << "#show.";
            show_ = true;
        }
    }

    void show_signature(std::string_view name, size_t arity) override {
        oss_ << "#show " << name << "/" << arity << ".";
    }

    void show_variable(var_t idx) override { oss_ << "#show " << vars_[idx] << "."; }

    auto add_variable(Clingo::Symbol var) -> var_t override {
        auto it = std::ranges::find(vars_, var);
        if (it == vars_.end()) {
            vars_.emplace_back(var);
            return static_cast<var_t>(vars_.size() - 1);
        }
        return static_cast<var_t>(it - vars_.begin());
    }

    auto add_constraint(lit_t lit, CoVarVec const &elems, val_t rhs, bool strict) -> bool override {
        oss_ << lit << (strict ? " <> " : " -> ");
        bool sep{false};
        for (auto const &[co, var] : elems) {
            oss_ << (sep ? " + " : "") << co << "*" << vars_[var];
            sep = true;
        }
        if (elems.empty()) {
            oss_ << "0";
        }
        oss_ << " <= " << rhs << ".";
        return true;
    }

    auto add_nonlinear(lit_t lit, val_t co_ab, var_t var_a, var_t var_b, val_t co_c, var_t var_c, val_t rhs,
                       bool strict) -> bool override {
        oss_ << lit << (strict ? " <> " : " -> ");
        if (co_ab != 0) {
            oss_ << co_ab << "*" << vars_[var_a] << "*" << vars_[var_b];
        }
        if (co_c != 0) {
            oss_ << (co_ab != 0 ? " + " : "") << co_c << "*" << vars_[var_c];
        }
        if (co_ab == 0 && co_c == 0) {
            oss_ << "0";
        }
        oss_ << " <= " << rhs << ".";
        return true;
    }

    void add_minimize(val_t co, var_t var) override { minimize_.emplace_back(co, var); }

    auto add_distinct(lit_t lit, std::vector<std::pair<CoVarVec, val_t>> const &elems) -> bool override {
        oss_ << lit << " -> ";
        bool sep{false};
        if (elems.size() > 1) {
            for (auto const &elem : elems) {
                if (sep) {
                    oss_ << " != ";
                }
                sep = true;
                bool plus{false};
                for (auto const &[co, var] : elem.first) {
                    oss_ << (plus ? " + " : "") << co << "*" << vars_[var];
                    plus = true;
                }
                if (elem.second != 0) {
                    oss_ << (plus ? " + " : "") << elem.second;
                }
            }
        } else {
            oss_ << "true";
        }
        oss_ << ".";
        return true;
    }

    auto add_disjoint(lit_t lit, CoVarVec const &elems) -> bool override {
        oss_ << lit << " -> ";
        bool sep{false};
        if (elems.size() > 1) {
            for (auto const &elem : elems) {
                if (sep) {
                    oss_ << " != ";
                }
                sep = true;
                oss_ << vars_[elem.second] << "@" << elem.first;
            }
        } else {
            oss_ << "true";
        }
        oss_ << ".";
        return true;
    }

    auto add_dom(lit_t lit, var_t var, IntervalSet<val_t> const &elems) -> bool override {
        oss_ << lit << " -> " << vars_[var] << " = { ";
        bool sep{false};
        for (auto const &[l, r] : elems) {
            oss_ << (sep ? ", " : "") << l << ".." << r;
            sep = true;
        }
        oss_ << "}.";
        return true;
    }

    auto get_or_add_cond_var(var_t orig_var, lit_t raw_cid) -> std::pair<var_t, bool> override {
        auto [it, inserted] = cond_vars_.try_emplace({orig_var, raw_cid});
        if (inserted) {
            vars_.emplace_back(Clingo::Number(static_cast<int>(vars_.size()) + 1));
            it->second = static_cast<var_t>(vars_.size() - 1);
        }
        return {it->second, inserted};
    }

    void commit() {
        if (!minimize_.empty()) {
            oss_ << "#minimize { ";
            bool sep{false};
            for (auto const &[co, var] : minimize_) {
                oss_ << (sep ? " + " : "") << co << "*" << vars_[var];
                sep = true;
            }
            oss_ << " }.";
        }
    }

  private:
    std::ostringstream &oss_;
    bool show_{false};
    lit_t literals_{2};
    std::vector<Clingo::Symbol> vars_;
    std::map<std::pair<var_t, lit_t>, var_t> cond_vars_;
    CoVarVec minimize_;
};

struct Fixture {
    using sret = std::pair<CoVarVec, val_t>;

    static auto simplify(CoVarVec const &vec, bool drop_zero = true) -> sret {
        CoVarVec ret = vec;
        auto rhs = Clingcon::simplify(ret, drop_zero);
        return {ret, rhs};
    }

    [[nodiscard]] auto transform(std::string_view str, bool shift = true) const -> std::string {
        std::ostringstream oss;
        Clingo::AST::parse(lib, str, [&](auto const &stm) {
            Clingcon::transform(
                lib, stm,
                [&](Clingo::AST::Node const &stm) {
                    if (stm.type() != Clingo::AST::NodeType::statement_program) {
                        oss << stm.to_string();
                    }
                },
                shift);
        });
        return oss.str();
    }

    [[nodiscard]] auto parse(std::string_view str) const -> std::string {
        Clingo::Control ctl{lib};
        ctl.parse_string(THEORY);
        {
            auto prg = Clingo::AST::Program{lib};
            Clingo::AST::parse(lib, str, [&](auto const &stm) {
                Clingcon::transform(lib, stm, [&](Clingo::AST::Node const &stm) { prg.add(stm); }, true);
            });
            ctl.join(prg);
        }
        ctl.ground();

        auto oss = std::ostringstream{};
        auto bld = TestBuilder{oss};
        std::ignore = Clingcon::parse(lib, bld, ctl.base().theory());
        bld.commit();
        return oss.str();
    }

    Clingo::Library lib;
};

} // namespace

TEST_CASE_METHOD(Fixture, "parsing simplify") {
    REQUIRE(simplify({}) == sret({}, 0));
    REQUIRE(simplify({{1, 0}, {1, 1}}) == sret({{1, 0}, {1, 1}}, 0));
    REQUIRE(simplify({{1, INVALID_VAR}}) == sret({}, -1));
    REQUIRE(simplify({{0, 0}}) == sret({}, 0));
    REQUIRE(simplify({{0, 0}, {0, 0}}, false) == sret({{0, 0}}, 0));
    REQUIRE(simplify({{0, 0}, {1, INVALID_VAR}, {2, INVALID_VAR}, {3, 0}, {4, 0}}) == sret({{7, 0}}, -3));

    REQUIRE_THROWS_AS(simplify({{std::numeric_limits<int>::max(), 0}, {std::numeric_limits<int>::max(), 0}}),
                      std::overflow_error);
    REQUIRE_THROWS_AS(simplify({{std::numeric_limits<int>::min(), INVALID_VAR}}), std::overflow_error);
}

TEST_CASE_METHOD(Fixture, "parsing transform") {
    REQUIRE(transform("&sum{ } = 0 :- &sum{ } = 1.") == "&__sum_h { } = 0 :- &__sum_b { } = 1.");
    REQUIRE(transform(":- &sum{ } = 0.") == "&__sum_h { } != 0.");
    REQUIRE(transform(":- &sum{ } = 0, &sum{ } = 1.") == "&__sum_h { } != 0 :- &__sum_b { } = 1.");

    REQUIRE(transform("&sum{ X } = 0.") == "&__sum_h { X } = 0.");
    REQUIRE(transform("&sum{ X : p(X,Y) } = 0.") == "&__sum_h { X,Y: p(X,Y) } = 0.");
    REQUIRE(transform("&sum{ X : p(X,Y); X : q(X,Y) } = 0.") == "&__sum_h { X,0,Y: p(X,Y); X,1,Y: q(X,Y) } = 0.");
    REQUIRE(transform("&sum{ X : p(X,_) } = 0.") == "&__sum_h { X,__CLINGCON_0: p(X,__CLINGCON_0) } = 0.");
    REQUIRE(transform("&sum { 1:p(1..10,X;Y) } <= 5.") ==
            "&__sum_h { 1,0,X,__CLINGCON_0: p(__CLINGCON_0,X), __CLINGCON_0=1..10; 1,1,Y: p(Y) } <= 5.");
    REQUIRE(transform("&sum { 1:1 < 1..10 < 15 } <= 5.") ==
            "&__sum_h { 1,__CLINGCON_0: 1<__CLINGCON_0<15, __CLINGCON_0=1..10 } <= 5.");
    REQUIRE(transform("&sum { X:p(@f) } <= 5.") ==
            "&__sum_h { X,__CLINGCON_0: p(__CLINGCON_0), __CLINGCON_0=@f } <= 5.");
    REQUIRE(transform("&sum { X:p((X;Y)+Z) } <= 5.") == "&__sum_h { X,0,Z: p(X+Z); X,1,Y,Z: p(Y+Z) } <= 5.");
    REQUIRE(transform("&sum { X:p(|X;Y|) } <= 5.") == "&__sum_h { X,0: p(|X|); X,1,Y: p(|Y|) } <= 5.");
    REQUIRE(transform("&sum { X:p(-(X;Y)) } <= 5.") == "&__sum_h { X,0: p(-X); X,1,Y: p(-Y) } <= 5.");
    REQUIRE(transform("&distinct{ p(Q) - p(P) : P < Q, P=1..o-1, Q=P+1..o }.") ==
            "&distinct { (p(Q) - p(P)),__CLINGCON_0,__CLINGCON_1: P<Q, P=__CLINGCON_0, Q=__CLINGCON_1, "
            "__CLINGCON_0=1..o-1, __CLINGCON_1=P+1..o }.");
}

TEST_CASE_METHOD(Fixture, "parsing sum head") {
    REQUIRE(parse("&sum { 7; 2**3 } >= 0.") == "2 -> 0 <= 15.");
    REQUIRE(parse("&sum { x; y; z } = 0.") == "2 -> 1*x + 1*y + 1*z <= 0."
                                              "2 -> -1*x + -1*y + -1*z <= 0.");
    REQUIRE(parse("&sum { x; y; z } != 0.") == "{ 3, 4, -2 }."
                                               "{ -3, -4 }."
                                               "{ 2, -3 }."
                                               "{ 2, -4 }."
                                               "3 -> 1*x + 1*y + 1*z <= -1."
                                               "4 -> -1*x + -1*y + -1*z <= -1.");
    REQUIRE(parse("&sum { x; y; z } <= 0.") == "2 -> 1*x + 1*y + 1*z <= 0.");
    REQUIRE(parse("&sum { x; y; z } < 0.") == "2 -> 1*x + 1*y + 1*z <= -1.");
    REQUIRE(parse("&sum { x; y; z } >= 0.") == "2 -> -1*x + -1*y + -1*z <= 0.");
    REQUIRE(parse("&sum { x; y; z } > 0.") == "2 -> -1*x + -1*y + -1*z <= -1.");
}

TEST_CASE_METHOD(Fixture, "parsing sum body") {
    REQUIRE(parse("a :- &sum { x; y; z } = 0.") == "{ -2, 3 }."
                                                   "{ -2, 4 }."
                                                   "{ -3, -4, 2 }."
                                                   "3 -> 1*x + 1*y + 1*z <= 0."
                                                   "-3 -> -1*x + -1*y + -1*z <= -1."
                                                   "4 -> -1*x + -1*y + -1*z <= 0."
                                                   "-4 -> 1*x + 1*y + 1*z <= -1.");
    REQUIRE(parse("a :- &sum { x; y; z } != 0.") == "{ 2, 3 }."
                                                    "{ 2, 4 }."
                                                    "{ -3, -4, -2 }."
                                                    "3 -> 1*x + 1*y + 1*z <= 0."
                                                    "-3 -> -1*x + -1*y + -1*z <= -1."
                                                    "4 -> -1*x + -1*y + -1*z <= 0."
                                                    "-4 -> 1*x + 1*y + 1*z <= -1.");
    REQUIRE(parse("a :- &sum { x; y; z } <= 0.") == "2 -> 1*x + 1*y + 1*z <= 0."
                                                    "-2 -> -1*x + -1*y + -1*z <= -1.");
    REQUIRE(parse("a :- &sum { x; y; z } < 0.") == "2 -> 1*x + 1*y + 1*z <= -1."
                                                   "-2 -> -1*x + -1*y + -1*z <= 0.");
    REQUIRE(parse("a :- &sum { x; y; z } >= 0.") == "2 -> -1*x + -1*y + -1*z <= 0."
                                                    "-2 -> 1*x + 1*y + 1*z <= -1.");
    REQUIRE(parse("a :- &sum { x; y; z } > 0.") == "2 -> -1*x + -1*y + -1*z <= -1."
                                                   "-2 -> 1*x + 1*y + 1*z <= 0.");
}

TEST_CASE_METHOD(Fixture, "parsing misc") {
    REQUIRE(parse("&sum { x + y + z } = 0.") == "2 -> 1*x + 1*y + 1*z <= 0."
                                                "2 -> -1*x + -1*y + -1*z <= 0.");
    REQUIRE(parse("&sum { 2 * (x + 3 * y) } <= z.") == "2 -> 2*x + 6*y + -1*z <= 0.");
}

TEST_CASE_METHOD(Fixture, "parsing diff") {
    REQUIRE(parse("&diff { x - z } <= 0.") == "2 -> 1*x + -1*z <= 0.");
    REQUIRE(parse("a :- &diff { x - z } <= 0.") == "2 -> 1*x + -1*z <= 0."
                                                   "-2 -> -1*x + 1*z <= -1.");
}

TEST_CASE_METHOD(Fixture, "parsing distinct") {
    REQUIRE(parse("&distinct { x; y; z }.") == "2 -> 1*x != 1*y != 1*z.");
    REQUIRE(parse("&distinct { x+y; 3*y+2; z; -1 }.") == "2 -> 1*x + 1*y != 3*y + 2 != 1*z != -1.");
}

TEST_CASE_METHOD(Fixture, "parsing disjoint") {
    REQUIRE(parse("&disjoint { x@10; y@1+11; z@ -10 }.") == "2 -> x@10 != y@12.");
}

TEST_CASE_METHOD(Fixture, "parsing show") {
    REQUIRE(parse("&show { x/1; y }.") == "#show."
                                          "#show x/1."
                                          "#show y.");
}

TEST_CASE_METHOD(Fixture, "parsing dom") {
    REQUIRE(parse("&dom { 1..2; 5; 10..12 } = x.") == "2 -> x = { 1..3, 5..6, 10..13}.");
}

TEST_CASE_METHOD(Fixture, "parsing optimize") {
    REQUIRE(parse("&minimize { x - z }.") == "#minimize { 1*x + -1*z }.");
    REQUIRE(parse("&maximize { x - z }.") == "#minimize { -1*x + 1*z }.");
}

TEST_CASE_METHOD(Fixture, "parsing nonlinear") {
    REQUIRE(parse("&nsum { 2*x*y + 3*z + 4 } <= 5.") == "2 -> 2*x*y + 3*z <= 1.");
    REQUIRE(parse("&nsum { (2**3)*x*y + (3**4)*z + (5**6) } <= 5.") == "2 -> 8*x*y + 81*z <= -15620.");
}

} // namespace Clingcon
