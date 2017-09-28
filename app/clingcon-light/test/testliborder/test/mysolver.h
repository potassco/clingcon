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
#include "order/solver.h"
#include <stdexcept>
#include <algorithm>


class MySolver : public order::CreatingSolver
{
public:
    using Literal = order::Literal;
    using LitVec = order::LitVec;
    MySolver() : lits_(2) {}
    bool isTrue(Literal l) const { return l==trueLit(); }
    bool isFalse(Literal l) const { return l==falseLit(); }
    bool isUnknown(Literal l) const { return (l!=trueLit() && l != falseLit()); }

    /// set the literal to true/false, precondition - must be unknown
    // precondition unknown ? or what happens on failure ?
    bool set(Literal){ return true; }

    std::size_t numVars() const { return lits_-1; }

    void makeRestFalse()
    {

    }

    virtual void createNewLiterals(uint64 )
    {

    }

    void freeze(Literal ) {}

    void intermediateVariableOutOfRange() const
    {
        throw std::runtime_error("Intermediate Variable out of bounds (32bit integer)");
    }




    Literal getNewLiteral(bool ) { /*std::cout << "create lit" << lits_ << std::endl;*/ return Literal(lits_++,false); }
    Literal trueLit() const { return Literal(1, false); }
    Literal falseLit() const { return ~trueLit(); }

    bool createClause(const LitVec& lvv)
    {
        LitVec lv(lvv);
        lv.erase(std::remove_if(lv.begin(), lv.end(),[&](const Literal& l){ return isFalse(l); }),lv.end());
        for (auto i : lv)
            if (isTrue(i))
                return true;
//                for (auto i : lv)
//                {
//                    std::cout << i << ", ";
//                }
//                std::cout << "0,";
//                std::cout << std::endl;
        clauses_.insert(clauses_.end(), lv.begin(), lv.end());
        clauses_.emplace_back(Literal::fromRep(0)); // place holder
        return true;
    }


    bool createCardinality(Literal , int , LitVec&&)
    {
        assert(false); /// not implemented for sat solver clasp yet
        return false;
    }

    bool setEqual(const Literal &a, const Literal &b)
    {
        if (!createClause({a,~b})) return false;
        if (!createClause({~a,b})) return false;
        return true;
    }

    void printDimacs(std::ostream& out)
    {
        out << "p cnf " << lits_-1 << " " << std::count(clauses_.begin(), clauses_.end(), Literal::fromRep(0)) << "\n";
        for (auto i : clauses_)
        {
            out << (i.sign() ? -int(i.var()) : int(i.var()));
            if (i.asUint()==0)
                out << "\n";
            else
                out << " ";
        }
        out << "1 0\n";
    }

    void addMinimize(order::Literal , int32 , unsigned int )
    {
        assert(false); /// not implemented for sat solver clasp yet
    }

    const LitVec& clauses() const { /*std::cout << std::endl;*/ return clauses_; }

private:
    std::size_t lits_;
    LitVec clauses_;


};

