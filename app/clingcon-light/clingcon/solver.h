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
#include "clingcon/platform.h"
#include "clingcon/variable.h"
#include <cassert>
#include <clingo.hh>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sstream>
#include <vector>

namespace clingcon
{

using Literal = Clingo::literal_t;
using Var = Clingo::atom_t;
using LitVec = std::vector< Literal >;

/// REWRITE
// No common Solver object
// The Creating Solver is now known as "Grounder" and can create literals and
// rules.
// It can not determine the truth value of literals
// The Incremental Solver is now known as "Solver" and can determine the truth
// value of literals
// at runtime and add new runtime literals
// --- No, PropagateControl has an assignment() which provides this
// functionality,
// no need to wrap it, unnecessary complex
// --- I still wrap it for completion, to be indepent of changes etc...
//
//

class BaseSolver
{
public:
    virtual Literal trueLit() const = 0;
    virtual Literal falseLit() const = 0;
    virtual bool isTrue(Literal l) const = 0;
    virtual bool isFalse(Literal l) const = 0;
    virtual bool isUnknown(Literal l) const = 0;
};

class Grounder : public BaseSolver
{
public:
    Grounder(Clingo::Backend c)
        : c_(c)
        , trueLit_(c.add_atom())
    {
        c_.rule(false, {Clingo::atom_t(std::abs(trueLit_))}, {}); // add a fact for trueLit_
    }

    /// creates a new literal, makes a choice rule for it
    Literal createNewLiteral()
    {
        Literal l = c_.add_atom();
        c_.rule(true, {Clingo::atom_t(std::abs(l))}, {});
        return l;
    }

    Literal trueLit() const { return trueLit_; }
    Literal falseLit() const { return -trueLit(); }

    /// very limited currently
    // TODO: any chance for other facts?
    bool isTrue(Literal l) const { return l == trueLit_; }
    bool isFalse(Literal l) const { return l == -trueLit_; }
    bool isUnknown(Literal l) const { return !isFalse(l) && !isTrue(l); }

    bool createClause(Clingo::LiteralSpan lvv)
    {
        // class Negate {
        // public:
        //    Literal operator ()(Literal const *id) const {
        //      return -(*id);
        //    }
        //};
        // c_.rule(false,Clingo::Span<Literal,Negate>(lvv),{});

        /// TODO: currently does a copy, can this be avoided ?
        LitVec v;
        v.reserve(lvv.size());
        for (auto i : lvv)
        {
            v.push_back(-i);
        }
        c_.rule(false, {}, {&v[0], v.size()});
        return !(lvv.size() == 1 && lvv[0] == -trueLit_);
    }

    bool setEqual(const Literal &a, const Literal &b)
    {
        createClause({a, -b});
        createClause({-a, b});
        return a != -b;
    }

    bool createCardinality(Literal v, int lb, LitVec &&lits)
    {
        /// TODO: use an Iterator to convert to Weight Literals with weight 1
        // c_.weight_rule(false,{Clingo::atom_t(std::abs(v))},lb,lits);
        std::vector< Clingo::WeightedLiteral > wlv;
        for (auto i : lits)
        {
            wlv.emplace_back(i, 1);
        }
        c_.weight_rule(false, {Clingo::atom_t(std::abs(v))}, lb, {&wlv[0], wlv.size()});
        return true;
    }

    void intermediateVariableOutOfRange() const
    {
        throw std::runtime_error("Intermediate Variable out of bounds (32bit integer)");
    }

    void addMinimize(Literal v, int32 weight, unsigned int level)
    {
        c_.minimize(level, {Clingo::WeightedLiteral(v, weight)});
    }

private:
    Clingo::Backend c_;
    Literal trueLit_;
};

class Solver : public BaseSolver
{
public:
    Solver(Literal trueLit)
        : c_({nullptr})
        , trueLit_(trueLit)
    {
    }

    void addWatch(Clingo::PropagateInit &init, Literal l) { init.add_watch(l); }


    /// call these functions before any other stuff with the object
    void beginPropagate(Clingo::PropagateControl c) { c_ = c; }

    /// during runtime
    void addWatch(Literal l)
    {
        assert(c_.to_c());
        c_.add_watch(l);
    }

    bool isTrue(Literal l) const
    {
        assert(c_.to_c());
        return c_.assignment().is_true(l);
    }
    bool isFalse(Literal l) const
    {
        assert(c_.to_c());
        return c_.assignment().is_false(l);
    }
    bool isUnknown(Literal l) const
    {
        assert(c_.to_c());
        return !isFalse(l) && !isTrue(l);
    }

    Literal trueLit() const { return trueLit_; }
    Literal falseLit() const { return -trueLit_; }
    Literal getNewLiteral()
    {
        assert(c_.to_c());
        return c_.add_literal();
    }

    bool addClause(Clingo::LiteralSpan lits)
    {
        assert(c_.to_c());
        return c_.add_clause(lits);
    }
    /// TODO: add addclause and stuff
private:
    Clingo::PropagateControl c_;
    Literal trueLit_;
};

} // namespace clingcon
