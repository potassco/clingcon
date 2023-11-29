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
#include <catch2/catch_test_macros.hpp>

#include <sstream>

using namespace Clingcon;

using sret = std::pair<CoVarVec, val_t>;

auto simplify(CoVarVec const &vec, bool drop_zero = true) -> sret {
    CoVarVec ret = vec;
    auto rhs = Clingcon::simplify(ret, drop_zero);
    return {ret, rhs};
}

auto transform(char const *prg, bool shift = true) -> std::string {
    std::ostringstream oss;
    Clingo::AST::parse_string(prg, [&](Clingo::AST::Node const &ast) {
        if (ast.type() != Clingo::AST::Type::Program) {
            transform(
                ast, [&](Clingo::AST::Node const &ast) { oss << ast; }, shift);
        }
    });
    return oss.str();
}

class TestBuilder : public Clingcon::AbstractConstraintBuilder {
  public:
    TestBuilder(std::ostringstream &oss) : oss_{oss} {}
    TestBuilder(TestBuilder const &) = delete;
    TestBuilder(TestBuilder &&) = delete;
    auto operator=(TestBuilder const &) -> TestBuilder & = delete;
    auto operator=(TestBuilder &&) -> TestBuilder & = delete;
    ~TestBuilder() override = default;

    auto solver_literal(lit_t literal) -> lit_t override {
        static_cast<void>(literal);
        return 2;
    }

    auto is_true(lit_t literal) -> bool override { return literal == 1; }

    auto add_literal() -> lit_t override { return ++literals_; }

    auto add_clause(Clingo::LiteralSpan clause) -> bool override {
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

    void show_signature(char const *name, size_t arity) override { oss_ << "#show " << name << "/" << arity << "."; }

    void show_variable(var_t idx) override { oss_ << "#show " << vars_[idx] << "."; }

    auto add_variable(Clingo::Symbol var) -> var_t override {
        auto it = std::find(vars_.begin(), vars_.end(), var);
        if (it == vars_.end()) {
            vars_.emplace_back(var);
            return vars_.size() - 1;
        }
        return it - vars_.begin();
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
    CoVarVec minimize_;
};

auto parse(char const *prg) -> std::string {
    Clingo::Control ctl;
    {
        Clingo::AST::ProgramBuilder builder{ctl};
        std::ostringstream oss;
        Clingo::AST::parse_string(prg, [&](Clingo::AST::Node const &ast) {
            transform(
                ast, [&](Clingo::AST::Node &&trans) { builder.add(trans); }, true);
        });
    }
    ctl.add("base", {}, THEORY);
    ctl.ground({{"base", {}}});

    std::ostringstream oss;
    TestBuilder builder{oss};
    static_cast<void>(parse(builder, ctl.theory_atoms()));
    builder.commit();
    return oss.str();
}

TEST_CASE("parsing", "[parsing]") {
    SECTION("simplify") {
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
    SECTION("transform") {
        REQUIRE(transform("&sum{ } = 0 :- &sum{ } = 1.") == "&__sum_h { } = 0 :- &__sum_b { } = 1.");
        REQUIRE(transform(":- &sum{ } = 0.") == "&__sum_h { } != 0.");
        REQUIRE(transform(":- &sum{ } = 0, &sum{ } = 1.") == "&__sum_h { } != 0 :- &__sum_b { } = 1.");

        REQUIRE(transform("&sum{ X } = 0.") == "&__sum_h { X } = 0.");
        REQUIRE(transform("&sum{ X : p(X,Y) } = 0.") == "&__sum_h { X,Y: p(X,Y) } = 0.");
        REQUIRE(transform("&sum{ X : p(X,Y); X : q(X,Y) } = 0.") == "&__sum_h { X,0,Y: p(X,Y); X,1,Y: q(X,Y) } = 0.");
        REQUIRE(transform("&sum{ X : p(X,_) } = 0.") == "&__sum_h { X: p(X,_) } = 0.");
    }
    SECTION("parse") {
        SECTION("sum head") {
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
        SECTION("sum body") {
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
        SECTION("sum misc") {
            REQUIRE(parse("&sum { x + y + z } = 0.") == "2 -> 1*x + 1*y + 1*z <= 0."
                                                        "2 -> -1*x + -1*y + -1*z <= 0.");
            REQUIRE(parse("&sum { 2 * (x + 3 * y) } <= z.") == "2 -> 2*x + 6*y + -1*z <= 0.");
        }
        SECTION("diff") {
            REQUIRE(parse("&diff { x - z } <= 0.") == "2 -> 1*x + -1*z <= 0.");
            REQUIRE(parse("a :- &diff { x - z } <= 0.") == "2 -> 1*x + -1*z <= 0."
                                                           "-2 -> -1*x + 1*z <= -1.");
        }
        SECTION("distinct") {
            REQUIRE(parse("&distinct { x; y; z }.") == "2 -> 1*x != 1*y != 1*z.");
            REQUIRE(parse("&distinct { x+y; 3*y+2; z; -1 }.") == "2 -> 1*x + 1*y != 3*y + 2 != 1*z != -1.");
        }
        SECTION("disjoint") { REQUIRE(parse("&disjoint { x@10; y@1+11; z@ -10 }.") == "2 -> x@10 != y@12."); }
        SECTION("show") {
            REQUIRE(parse("&show { x/1; y }.") == "#show."
                                                  "#show x/1."
                                                  "#show y.");
        }
        SECTION("dom") { REQUIRE(parse("&dom { 1..2; 5; 10..12 } = x.") == "2 -> x = { 1..3, 5..6, 10..13}."); }
        SECTION("optimize") {
            REQUIRE(parse("&minimize { x - z }.") == "#minimize { 1*x + -1*z }.");
            REQUIRE(parse("&maximize { x - z }.") == "#minimize { -1*x + 1*z }.");
        }
        SECTION("nonlinear") {
            REQUIRE(parse("&nsum { 2*x*y + 3*z + 4 } <= 5.") == "2 -> 2*x*y + 3*z <= 1.");
            REQUIRE(parse("&nsum { (2**3)*x*y + (3**4)*z + (5**6) } <= 5.") == "2 -> 8*x*y + 81*z <= -15620.");
        }
    }
}
