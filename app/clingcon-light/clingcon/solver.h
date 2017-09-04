// {{{ MIT License

// Copyright 2017 Max Ostrowski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#pragma once
#include <iostream>
#include <sstream>
#include <ostream>
#include <cstdlib>
#include <vector>
#include <cassert>
#include <clingo.hh>
#include "clingcon/platform.h"
#include "clingcon/variable.h"

namespace clingcon
{


class Literal {
public:
    //! The default constructor creates the positive literal of the special sentinel var.
    //Literal() : rep_(0) { }

    //! Creates a literal of the variable var with sign s.
    /*!
     * \param var The literal's variable.
     * \param s true if new literal should be negative.
     */
    Literal(clingo_atom_t var, bool sign) : rep_(sign ? -var : var)  {
    }

    //! Returns the clingo representation of this literal.
    clingo_literal_t rep() const { return rep_; }

    //! Creates a literal from a clingo_literal_t.
    static Literal fromRep(clingo_literal_t rep) { return Literal(rep); }

    //! Returns the variable of the literal.
    uint32_t var() const { return std::abs(rep_); }

    //! Returns the sign of the literal.
    /*!
     * \return true if the literal is negative. Otherwise false.
     */

    bool sign() const { return rep_ < 0; }

    void swap(Literal& other) { std::swap(rep_, other.rep_); }

    //! Returns the complementary literal of this literal.
    /*!
     *  The complementary Literal of a Literal is a Literal referring to the
     *  same variable but with inverted sign.
     */
    inline Literal operator~() const {
        return Literal( rep_*-1); 
    }

    //! Equality-Comparison for literals.
    /*!
     * Two Literals p and q are equal, iff
     * - they both refer to the same variable
     * - they have the same sign
     * .
     */
    inline bool operator==(const Literal& rhs) const {
        return rep_ == rhs.rep_;
    }
    inline bool operator!=(const Literal& rhs) const {
        return rep_ != rhs.rep_;
    }
    inline bool operator<(const Literal& rhs) const {
        return rep_ < rhs.rep_;
    }
private:
    Literal(clingo_literal_t rep) : rep_(rep) {}
    clingo_literal_t rep_;
};

using LitVec = std::vector<Literal>;

/// REWRITE
// No common Solver object
// The Creating Solver is now known as "Grounder" and can create literals and rules.
// It can not determine the truth value of literals
// The Incremental Solver is now known as "Solver" and can determine the truth value of literals
// at runtime and add new runtime literals
// --- No, PropagateControl has an assignment() which provides this functionality,
// no need to wrap it, unnecessary complex
//
//

inline std::ostream& operator<< (std::ostream& stream, const Literal& l)
{
    stream << (l.sign() ? '-' : ' ') << l.var();
    return stream;
}


class Grounder
{
public:
    Grounder(Clingo::Backend& c) : c_(c), trueLit_(Literal(c.add_atom(),false))
    {
       c_.rule(false,{trueLit_},{}); // add a fact for trueLit_
    }

    Literal getNewLiteral()
    {
        return Literal(c_.add_atom(),false);
    }

    Literal trueLit() const { return trueLit_; }
    Literal falseLit() const { return ~trueLit(); }

    void createClause(const LitVec& lvv)
    {
        //class Negate {
        //public:
        //    Literal operator ()(Literal const *id) const {
        //      return ~(*id);
        //    }
        //};
        //c_.rule(false,Clingo::Span<Literal,Negate>(lvv),{});

        ///TODO: currently does a copy, can this be avoided ?
        LitVec v;
        v.reserve(lvv.size());
        for (auto i : lvv)
        {
            v.push_back(~i);
        }
        c_.rule(false,lvv,{});
    }

    void setEqual(const Literal &a, const Literal &b)
    {
        createClause({a,~b});
        createClause({~a,b});
    }

    void createCardinality(Literal v, int lb, LitVec &&lits)
    {
        /// TODO: use an Iterator to convert to Weight Literals with weight 1
        c_.weight_rule(false,{v},lb,lits);
    }

    void intermediateVariableOutOfRange() const
    {
        throw std::runtime_error("Intermediate Variable out of bounds (32bit integer)");
    }

    void addMinimize(Literal v, int32 weight, unsigned int level)
    {
        c_.minimize(level,{Clingo::WeightedLiteral(v.rep(),weight)});
    }

private:

    Clingo::Backend& c_;
    Literal trueLit_;
};


} // namespace clingcon



