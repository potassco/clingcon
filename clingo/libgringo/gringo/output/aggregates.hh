// {{{ MIT License

// Copyright 2017 Roland Kaminski

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

#ifndef _GRINGO_OUTPUT_AGGREGATES_HH
#define _GRINGO_OUTPUT_AGGREGATES_HH

#include <gringo/terms.hh>
#include <gringo/domain.hh>
#include <gringo/intervals.hh>
#include <gringo/output/literal.hh>

namespace Gringo { namespace Output {

struct TupleId;
using BodyAggregateElements = UniqueVec<std::pair<TupleId, Formula>, HashFirst<TupleId>, EqualToFirst<TupleId>>;
using HeadFormula = std::vector<std::pair<LiteralId, ClauseId>>;
using HeadAggregateElements = UniqueVec<std::pair<TupleId, HeadFormula>, HashFirst<TupleId>, EqualToFirst<TupleId>>;
using DisjunctiveBounds = IntervalSet<Symbol>;
using Interval = DisjunctiveBounds::Interval;
using ConjunctiveBounds = std::vector<std::pair<Interval, Interval>>;
using PlainBounds = std::vector<std::pair<Relation, Symbol>>;
using LitValVec = std::vector<std::pair<LiteralId, Symbol>>;
using LitUintVec = std::vector<std::pair<LiteralId, unsigned>>;

struct AggregateAnalyzer {
    enum Monotonicity { MONOTONE, ANTIMONOTONE, CONVEX, NONMONOTONE };
    enum WeightType { MIXED, POSITIVE, NEGATIVE };
    enum Truth { True, False, Open };
    using ConjunctiveBounds = std::vector<std::pair<Interval, Interval>>;

    AggregateAnalyzer(DomainData &data, NAF naf, DisjunctiveBounds const &disjunctiveBounds, AggregateFunction fun, Interval range, BodyAggregateElements const &elems);
    void print(std::ostream &out);
    LitValVec translateElems(DomainData &data, Translator &x, AggregateFunction fun, BodyAggregateElements const &bdElems, bool incomplete);

    Monotonicity monotonicity;
    WeightType weightType;
    Truth truth;
    ConjunctiveBounds bounds;
    Interval range;
};

inline Symbol getNeutral(AggregateFunction fun) {
    switch (fun) {
        case AggregateFunction::COUNT:
        case AggregateFunction::SUMP:
        case AggregateFunction::SUM: { return Symbol::createNum(0); }
        case AggregateFunction::MIN: { return Symbol::createSup(); }
        case AggregateFunction::MAX: { return Symbol::createInf(); }
    }
    assert(false);
    return {};
}

LiteralId getEqualClause(DomainData &data, Translator &x, std::pair<Id_t, Id_t> clause, bool conjunctive, bool equivalence);
LiteralId getEqualFormula(DomainData &data, Translator &x, Formula const &formula, bool conjunctive, bool equivalence);
LiteralId getEqualAggregate(DomainData &data, Translator &x, AggregateFunction fun, NAF naf, DisjunctiveBounds const &bounds, Interval const &range, BodyAggregateElements const &bdElems, bool recursive);

class MinMaxTranslator {
public:
    LiteralId translate(DomainData &data, Translator &x, AggregateAnalyzer &res, bool isMin, LitValVec &&elems, bool incomplete);
};

struct SumTranslator {
    SumTranslator() { }
    void addLiteral(DomainData &data, LiteralId const &lit, Potassco::Weight_t weight, bool recursive);
    void translate(DomainData &data, Translator &x, LiteralId const &head, Potassco::Weight_t bound, LitUintVec const &litsPosRec, LitUintVec const &litsNegRec, LitUintVec const &litsPosStrat, LitUintVec const &litsNegStrat);
    LiteralId translate(DomainData &data, Translator &x, ConjunctiveBounds &bounds, bool convex, bool invert);

    LitUintVec litsPosRec;
    LitUintVec litsNegRec;
    LitUintVec litsPosStrat;
    LitUintVec litsNegStrat;
};

} } // namespace Output Gringo

#endif // _GRINGO_OUTPUT_AGGREGATES_HH
