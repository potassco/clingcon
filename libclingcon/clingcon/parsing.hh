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

#ifndef CLINGCON_PARSING_H
#define CLINGCON_PARSING_H

#include "clingcon/base.hh"
#include "clingcon/util.hh"

//! @file clingcon/parsing.hh
//! This module contains functions for parsing and normalizing constraints.
//!
//! @author Roland Kaminski

namespace Clingcon {

//! This theory has to be loaded to use CSP constraints.
constexpr char const *THEORY = R"(
#theory cp {
    var_term  { };
    sum_term {
    -  : 3, unary;
    ** : 2, binary, right;
    *  : 1, binary, left;
    /  : 1, binary, left;
    \  : 1, binary, left;
    +  : 0, binary, left;
    -  : 0, binary, left
    };
    dom_term {
    -  : 4, unary;
    ** : 3, binary, right;
    *  : 2, binary, left;
    /  : 2, binary, left;
    \  : 2, binary, left;
    +  : 1, binary, left;
    -  : 1, binary, left;
    .. : 0, binary, left
    };
    disjoint_term {
    -  : 4, unary;
    ** : 3, binary, right;
    *  : 2, binary, left;
    /  : 2, binary, left;
    \  : 2, binary, left;
    +  : 1, binary, left;
    -  : 1, binary, left;
    @  : 0, binary, left
    };
    &__diff_h/0 : sum_term, {<=}, sum_term, any;
    &__diff_b/0 : sum_term, {<=}, sum_term, any;
    &__sum_h/0 : sum_term, {<=,=,!=,<,>,>=}, sum_term, any;
    &__sum_b/0 : sum_term, {<=,=,!=,<,>,>=}, sum_term, any;
    &__nsum_h/0 : sum_term, {<=,=,!=,<,>,>=}, sum_term, any;
    &__nsum_b/0 : sum_term, {<=,=,!=,<,>,>=}, sum_term, any;
    &minimize/0 : sum_term, directive;
    &maximize/0 : sum_term, directive;
    &show/0 : sum_term, directive;
    &distinct/0 : sum_term, head;
    &disjoint/0 : disjoint_term, head;
    &dom/0 : dom_term, {=}, var_term, head
}.
)";

//! CSP builder to use with the Clingcon::parse_theory function.
class AbstractConstraintBuilder {
  public:
    AbstractConstraintBuilder() = default;
    AbstractConstraintBuilder(AbstractConstraintBuilder const &) = delete;
    AbstractConstraintBuilder(AbstractConstraintBuilder &&) noexcept = delete;
    auto operator=(AbstractConstraintBuilder const &) -> AbstractConstraintBuilder & = delete;
    auto operator=(AbstractConstraintBuilder &&) noexcept -> AbstractConstraintBuilder & = delete;
    virtual ~AbstractConstraintBuilder() = default;

    //! Map a program to a solver literal.
    [[nodiscard]] virtual auto solver_literal(lit_t literal) -> lit_t = 0;
    //! Add a new solver literal.
    [[nodiscard]] virtual auto add_literal() -> lit_t = 0;
    //! Check whether the given solver literal is true.
    [[nodiscard]] virtual auto is_true(lit_t literal) -> bool = 0;
    //! Add a clause over solver literals.
    [[nodiscard]] virtual auto add_clause(Clingo::LiteralSpan clause) -> bool = 0;
    //! Inform the builder that there is a show statement.
    virtual void add_show() = 0;
    //! Show variables with the given signature.
    virtual void show_signature(char const *name, size_t arity) = 0;
    //! Show the given variable.
    virtual void show_variable(var_t idx) = 0;
    //! Get the integer representing a variable.
    [[nodiscard]] virtual auto add_variable(Clingo::Symbol var) -> var_t = 0;
    //! Add a constraint.
    [[nodiscard]] virtual auto add_constraint(lit_t lit, CoVarVec const &elems, val_t rhs, bool strict) -> bool = 0;
    //! Add a non-linear sum constraint.
    [[nodiscard]] virtual auto add_nonlinear(lit_t lit, val_t co_ab, var_t var_a, var_t var_b, val_t co_c, var_t var_c,
                                             val_t rhs, bool strict) -> bool = 0;
    //! Extend the minimize constraint.
    virtual void add_minimize(val_t co, var_t var) = 0;
    //! Add a distinct constraint.
    [[nodiscard]] virtual auto add_distinct(lit_t lit, std::vector<std::pair<CoVarVec, val_t>> const &elems)
        -> bool = 0;
    //! Add a disjoint constraint.
    [[nodiscard]] virtual auto add_disjoint(lit_t lit, CoVarVec const &elems) -> bool = 0;
    //! Add a domain for the given variable.
    [[nodiscard]] virtual auto add_dom(lit_t lit, var_t var, IntervalSet<val_t> const &elems) -> bool = 0;
};

//! Combine coefficients of terms with the same variable and optionally drop
//! zero weights and sum up terms without a variable.
//!
//! This functions throws if there is a (potential) overflow.
[[nodiscard]] auto simplify(CoVarVec &vec, bool drop_zero = true) -> val_t;

using NodeCallback = std::function<void(Clingo::AST::Node &&ast)>;

//! Transform the given statement with csp constraints and pass it on to the
//! given callback.
//!
//! Optionally shifts constraints from rule bodies into heads of integrity
//! constraints if possible.
void transform(Clingo::AST::Node const &ast, NodeCallback const &cb, bool shift);

//! Parse the given theory passing the result to the given builder.
//!
//! This functions throws if there is a (potential) overflow.
[[nodiscard]] auto parse(AbstractConstraintBuilder &builder, Clingo::TheoryAtoms theory_atoms) -> bool;

//! Check if the theory term has the given signature.
[[nodiscard]] auto match(Clingo::TheoryTerm const &term, char const *name, size_t arity) -> bool;

} // namespace Clingcon

#endif // CLINGCON_PARSING_H
