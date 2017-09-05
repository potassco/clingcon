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
#include "clingcon/solver.h"
#include "clingcon/normalizer.h"
#include "clingcon/constraint.h"
#include <unordered_map>
#include <sstream>



namespace clingcon
{

template <class T>
inline void hash_combine(std::size_t & seed, const T & v)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}


using NameList = std::unordered_map<Variable,std::pair<std::string,LitVec>>;


class TheoryParser
{
public:

    enum CType {SUM, DOM, DISTINCT, SHOW, MINIMIZE};
    using mytuple = std::vector<Clingo::id_t>;   /// a tuple identifier
    using tuple2View = std::map<mytuple, View>; // could be unordered


    TheoryParser(Normalizer& n, Clingo::TheoryAtoms td, Literal trueLit) :
        n_(n), td_(td), trueLit_(trueLit)
    {}
    bool isClingconConstraint(Clingo::TheoryAtomIterator& i);
    bool isUnarySum(Clingo::TheoryAtomIterator& i);

    /// returns false, if not a constraint of this theory
    /// throws string with error if error occurs
    /// save constraint as strict or nonstrict
    bool readConstraint(Clingo::TheoryAtomIterator& i, Direction dir);
    /// turn show predicates to variables
    NameList& postProcess();
    const std::vector<tuple2View>& minimize() const;

    void reset();

    /// slow lookup, use with care
    std::string getName(Variable v);

private:

    void error(const std::string& s);
    void error(const std::string& s, Clingo::id_t id);


    bool getConstraintType(Clingo::id_t id, CType& t);
    bool getGuard(Clingo::id_t id, LinearConstraint::Relation& rel);


    std::string toString(const Clingo::TheoryTerm& t)
    {
        std::stringstream ss;
        return toString(ss,t).str();
    }

    std::stringstream& toString(std::stringstream& ss, const Clingo::TheoryTerm& t);

    bool isNumber(Clingo::id_t id);
    bool isNumber(const Clingo::TheoryTerm& a);

    int getNumber(Clingo::id_t id);
    int getNumber(const Clingo::TheoryTerm& a);

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
    ///        operator + binary -> one is number, other getView or both is number -> create Var
    ///        operator - binary -> one is number, other getView or both is number -> create Var
    ///        operator * binary -> one is number, other getView or both is number -> create Var
    ///
    bool getView(Clingo::id_t id, View& v);

    View createVar(Clingo::id_t id);

    View createVar(Clingo::id_t id, int32 val);


private:

    bool check(Clingo::id_t id);
    bool isVariable(Clingo::id_t id);


    std::unordered_map<Clingo::id_t, std::pair<CType,bool>>  termId2constraint_;
    std::unordered_map<Clingo::id_t, LinearConstraint::Relation>  termId2guard_;
    std::vector<std::pair<Clingo::id_t,Literal>> shown_; /// Variable to TermId + condition literal
    std::vector<std::pair<Clingo::id_t,Literal>> shownPred_; /// a list of p/3 predicates to be shown + condition literal
    std::vector<tuple2View> minimize_;                /// for each level

    std::vector<View>  termId2View_;
    Normalizer& n_;
    Clingo::TheoryAtoms td_;
    Literal trueLit_;
    const Clingo::id_t MAXID = std::numeric_limits<Clingo::id_t>::max();
    std::unordered_map<std::string, View> string2view_;
    using Predicate = std::pair<std::string,unsigned int>;

    struct PredicateHasher
    {
        std::size_t operator()(Predicate const& p) const
        {
        std::size_t seed = 0;
        hash_combine(seed, p.first);
        hash_combine(seed, p.second);
        return seed;
        }
    };

    std::unordered_map<Predicate,std::set<Variable>,PredicateHasher> pred2Variables_;
    std::unordered_map<Variable,std::pair<std::string,LitVec>> orderVar2nameAndConditions_;
    std::unordered_map<Predicate,LitVec,PredicateHasher> shownPredPerm_;
};

}