// {{{ MIT License

// Copyright 2018 Max Ostrowski

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
#include "clingcon/constraint.h"
#include "clingcon/normalizer.h"
#include "clingcon/solver.h"
#include <sstream>
#include <unordered_map>

namespace clingcon
{

template < class T >
inline void hash_combine(std::size_t &seed, const T &v)
{
    std::hash< T > hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

using NameList = std::unordered_map< Variable, std::pair< Clingo::Symbol, LitVec > >;
using SymbolMap = std::unordered_map< Clingo::Symbol, View >;

class TheoryParser
{
public:
    enum CType
    {
        SUM,
        DOM,
        DISTINCT,
        SHOW,
        MINIMIZE
    };
    using mytuple = std::vector< Clingo::id_t >;  /// a tuple identifier
    using tuple2View = std::map< mytuple, View >; // could be unordered

    TheoryParser(Grounder &g, Normalizer &n, Clingo::TheoryAtoms td)
        : s_(g)
        , n_(n)
        , td_(td)
        , MAXID(Clingo::Infimum())
    {
    }


    bool readConstraints();
    /// turn show predicates to variables
    NameList &postProcess();
    const std::vector< tuple2View > &minimize() const;

    void reset();

    const SymbolMap& getSymbols() const;
    /// slow lookup, use with care
    Clingo::Symbol getSymbol(Variable v);
    const char *getName(Variable v) const;

private:
    bool isClingconConstraint(Clingo::TheoryAtom &i);
    bool isUnarySum(Clingo::TheoryAtom &i);

    /// returns false, if not a constraint of this theory
    /// throws string with error if error occurs
    /// save constraint as strict or nonstrict
    bool readConstraint(Clingo::TheoryAtom &i, Direction dir);

    void error(const std::string &s);
    void error(const std::string &s, const Clingo::TheoryTerm &t);

    bool getConstraintType(const Clingo::TheoryTerm &term, CType &t);
    bool getGuard(const char *c, LinearConstraint::Relation &rel);

    std::string toString(const Clingo::TheoryTerm &t);
    std::stringstream &toString(std::stringstream &ss, const Clingo::TheoryTerm &t);
    Clingo::Symbol toSymbol(const Clingo::TheoryTerm &t) const;


    bool isNumber(const Clingo::TheoryTerm &a) const;

    int getNumber(const Clingo::TheoryTerm &a) const;

    void add2Shown(Variable v, Clingo::Symbol tid, Literal l);

    ///
    /// \brief getView
    /// \param id
    /// \return
    /// either number -> create new var
    /// string -> create var
    /// tuple -> not allowed
    /// function
    ///        named function -> eval and create var
    ///        operator + unary ->getView of Rest
    ///        operator - unary ->getView of Rest
    ///        operator + binary -> one is number, other getView or both is
    ///        number
    ///        -> create Var
    ///        operator - binary -> one is number, other getView or both is
    ///        number
    ///        -> create Var
    ///        operator * binary -> one is number, other getView or both is
    ///        number
    ///        -> create Var
    ///
    bool getView(const Clingo::TheoryTerm &a, View &v);

    View createVar(const Clingo::TheoryTerm &t);
    View createVar(const Clingo::Symbol &t);

private:
    bool check(const Clingo::TheoryTerm &id);
    bool isVariable(const Clingo::TheoryTerm &id);


    Grounder &s_;

    std::unordered_map< Clingo::id_t, CType > termId2constraint_;
    using Predicate = std::pair< Clingo::Symbol, size_t >;
    std::vector< std::pair< Clingo::Symbol, Literal > >
        shown_; /// Variable to Symbol + condition literal
    std::vector< std::pair< Predicate, Literal > >
        shownPred_; /// a list of symbol/arity predicates to be shown + condition literal
    std::vector< tuple2View > minimize_; /// for each level

    Normalizer &n_;
    Clingo::TheoryAtoms td_;
    const Clingo::Symbol MAXID;
    SymbolMap symbol2view_;


    struct PredicateHasher
    {
        std::size_t operator()(Predicate const &p) const
        {
            std::size_t seed = 0;
            hash_combine(seed, p.first);
            hash_combine(seed, p.second);
            return seed;
        }
    };

    std::unordered_map< Predicate, std::set< Variable >, PredicateHasher > pred2Variables_;
    std::unordered_map< Variable, std::pair< Clingo::Symbol, LitVec > > orderVar2nameAndConditions_;
    std::unordered_map< Predicate, LitVec, PredicateHasher > shownPredPerm_;
};
}
