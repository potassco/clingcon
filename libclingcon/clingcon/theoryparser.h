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

using NameList = std::unordered_map< Variable, std::pair< std::string, LitVec > >;

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
    {
    }


    bool readConstraints();
    /// turn show predicates to variables
    NameList &postProcess();
    const std::vector< tuple2View > &minimize() const;

    void reset();

    /// slow lookup, use with care
    std::string getName(Variable v);

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

    bool isNumber(const Clingo::TheoryTerm &a);

    int getNumber(const Clingo::TheoryTerm &a);

    void add2Shown(Variable v, uint32 tid, Literal l);

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

    View createVar(const Clingo::TheoryTerm &t, int32 val);

private:
    bool check(const Clingo::TheoryTerm &id);
    bool isVariable(const Clingo::TheoryTerm &id);

    Grounder &s_;

    std::unordered_map< Clingo::id_t, CType > termId2constraint_;
    std::vector< std::pair< Clingo::id_t, Literal > >
        shown_; /// Variable to TermId + condition literal
    std::vector< std::pair< Clingo::id_t, Literal > >
        shownPred_; /// a list of p/3 predicates to be shown + condition literal
    std::vector< tuple2View > minimize_; /// for each level

    std::vector< View > termId2View_;
    Normalizer &n_;
    Clingo::TheoryAtoms td_;
    const Clingo::id_t MAXID = std::numeric_limits< Clingo::id_t >::max();
    std::unordered_map< std::string, View > string2view_;
    using Predicate = std::pair< std::string, unsigned int >;

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
    std::unordered_map< Variable, std::pair< std::string, LitVec > > orderVar2nameAndConditions_;
    std::unordered_map< Predicate, LitVec, PredicateHasher > shownPredPerm_;
};
}
