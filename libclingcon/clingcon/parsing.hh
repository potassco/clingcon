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
    \\ : 1, binary, left;
    +  : 0, binary, left;
    -  : 0, binary, left
    };
    dom_term {
    -  : 4, unary;
    ** : 3, binary, right;
    *  : 2, binary, left;
    /  : 2, binary, left;
    \\ : 2, binary, left;
    +  : 1, binary, left;
    -  : 1, binary, left;
    .. : 0, binary, left
    };
    &sum/1 : sum_term, {<=,=,!=,<,>,>=}, sum_term, any;
    &diff/1 : sum_term, {<=}, sum_term, any;
    &minimize/0 : sum_term, directive;
    &maximize/0 : sum_term, directive;
    &show/0 : sum_term, directive;
    &distinct/0 : sum_term, head;
    &dom/0 : dom_term, {=}, var_term, head
}.
)";

//! CSP builder to use with the Clingcon::parse_theory function.
class AbstractConstraintBuilder {
public:
    AbstractConstraintBuilder() = default;
    AbstractConstraintBuilder(AbstractConstraintBuilder const &) = delete;
    AbstractConstraintBuilder(AbstractConstraintBuilder &&) noexcept = delete;
    AbstractConstraintBuilder& operator=(AbstractConstraintBuilder const &) = delete;
    AbstractConstraintBuilder& operator=(AbstractConstraintBuilder &&) noexcept = delete;
    virtual ~AbstractConstraintBuilder() = default;

    //! Return an AbstractClauseCreator.
    virtual AbstractClauseCreator &cc() = 0;
    //! Inform the builder that there is a show statement.
    virtual void add_show() = 0;
    //! Show variables with the given signature.
    virtual void show_signature(char const *name, size_t arity) = 0;
    //! Show the given variable.
    virtual void show_variable(var_t idx) = 0;
    //! Get the integer representing a variable.
    virtual var_t add_variable(Clingo::Symbol var) = 0;
    //! Add a constraint.
    virtual void add_constraint(lit_t lit, CoVarVec const &elems, val_t rhs, bool strict) = 0;
    //! Extend the minimize constraint.
    virtual void add_minimize(val_t co, var_t var) = 0;
    //! Add a distinct constraint.
    virtual void add_distinct(lit_t lit, std::vector<CoVarVec> const &elems) = 0;
    //! Add a domain for the given variable.
    virtual void add_dom(lit_t lit, var_t var, std::vector<std::pair<val_t, val_t>>) = 0;
};

//! Combine coefficients of terms with the same variable and optionally drop
//! zero weights and sum up terms without a variable.
//!
//! This functions throws if there is a (potential) overflow.
[[nodiscard]] val_t simplify(CoVarVec &vec, bool drop_zero=true);

//! Transform the program with csp constraints in the given file and pass it to
//! the builder.
void transform(Clingo::ProgramBuilder &builder, char const *prg, bool shift);

//! Parse the given theory passing the result to the given builder.
//!
//! This functions throws if there is a (potential) overflow.
void parse_theory(AbstractConstraintBuilder &builder, Clingo::TheoryAtoms &theory_atoms);

} // namespace Clingcon

#endif // CLINGCON_PARSING_H
