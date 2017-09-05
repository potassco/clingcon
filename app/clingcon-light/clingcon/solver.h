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


using Literal = Clingo::literal_t;
using LitVec = std::vector<Literal>;

/// REWRITE
// No common Solver object
// The Creating Solver is now known as "Grounder" and can create literals and rules.
// It can not determine the truth value of literals
// The Incremental Solver is now known as "Solver" and can determine the truth value of literals
// at runtime and add new runtime literals
// --- No, PropagateControl has an assignment() which provides this functionality,
// no need to wrap it, unnecessary complex
// --- I still wrap it for completion, to be indepent of changes etc...
//
//


class Grounder
{
public:
    Grounder(Clingo::Backend& c) : c_(c), trueLit_(c.add_atom())
    {
       c_.rule(false,{Clingo::atom_t(std::abs(trueLit_))},{}); // add a fact for trueLit_
    }

    /// creates a new literal, makes a choice rule for it
    Literal createNewLiteral()
    {
        Literal l = c_.add_atom();
        c_.rule(true,{Clingo::atom_t(std::abs(l))},{});
        return l;
    }

    Literal trueLit() const { return trueLit_; }
    Literal falseLit() const { return ~trueLit(); }

    void createClause(const LitVec& lvv)
    {
        //class Negate {
        //public:
        //    Literal operator ()(Literal const *id) const {
        //      return -(*id);
        //    }
        //};
        //c_.rule(false,Clingo::Span<Literal,Negate>(lvv),{});

        ///TODO: currently does a copy, can this be avoided ?
        LitVec v;
        v.reserve(lvv.size());
        for (auto i : lvv)
        {
            v.push_back(-i);
        }
        c_.rule(false,{},{&lvv[0],lvv.size()});
    }

    void setEqual(const Literal &a, const Literal &b)
    {
        createClause({a,-b});
        createClause({-a,b});
    }

    void createCardinality(Literal v, int lb, LitVec &&lits)
    {
        /// TODO: use an Iterator to convert to Weight Literals with weight 1
        //c_.weight_rule(false,{Clingo::atom_t(std::abs(v))},lb,lits);
        std::vector<Clingo::WeightedLiteral> wlv;
        for (auto i : lits) {
            wlv.emplace_back(i,1);
        }
        c_.weight_rule(false,{Clingo::atom_t(std::abs(v))},lb,{&wlv[0],wlv.size()});
    }

    void intermediateVariableOutOfRange() const
    {
        throw std::runtime_error("Intermediate Variable out of bounds (32bit integer)");
    }

    void addMinimize(Literal v, int32 weight, unsigned int level)
    {
        c_.minimize(level,{Clingo::WeightedLiteral(v,weight)});
    }

private:

    Clingo::Backend& c_;
    Literal trueLit_;
};


class Solver
{
public:
    Solver(Literal trueLit) : c_(nullptr), trueLit_(trueLit) {}
    /// call these functions before any other stuff with the object
    void beginPropagate(Clingo::PropagateControl& c)
    {
       assert(c_==nullptr);
       c_ = &c;
    }

    void endPropagate()
    {
       assert(c_!=nullptr);
       c_=nullptr;
    }
    bool isTrue(Literal l) {
       return c_->assignment().is_true(l);
    }
    bool isFalse(Literal l) {
       return c_->assignment().is_false(l);
    }
    bool isUnknown(Literal l) {
       return !isFalse(l) && ! isTrue(l);
    }

    Literal trueLit() { return trueLit_; }
    Literal falseLit() { return ~trueLit_; }
    Literal getNewLiteral() { return c_->add_literal(); }
    ///TODO: add addclause and stuff
private:
    Clingo::PropagateControl* c_;
    Literal trueLit_;
};


} // namespace clingcon


