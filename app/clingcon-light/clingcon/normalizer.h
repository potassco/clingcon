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
#include <clingcon/config.h>
#include <clingcon/constraint.h>
#include <clingcon/linearpropagator.h>
#include <clingcon/storage.h>
#include <map>

namespace clingcon
{

/// contains the orderLiterals, either need to be moved or Normalizer to be kept
class Normalizer
{
public:
    Normalizer(Grounder &s, Config conf)
        : s_(s)
        , vc_(s, conf)
        , conf_(conf)
        , firstRun_(true)
        , varsBefore_(0)
        , varsAfter_(0)
        , varsAfterFinalize_(0)
    {
    }

    /// can be made const, only changed for unit tests
    Config &getConfig() { return conf_; }

    /// to create view with an already existing variable, just use View manually
    View createView() { return View(vc_.createVariable()); }
    View createView(const Domain &d) { return View(vc_.createVariable(d)); }

    /// adds a constraint to the propagator l.v <-> l.l
    /// returns false on unsat
    void addConstraint(ReifiedLinearConstraint &&l);
    void addConstraint(ReifiedDomainConstraint &&d);
    void addConstraint(ReifiedAllDistinct &&l);
    void addMinimize(View &v, unsigned int level);

    /// do initial propagation
    bool prepare();

    /// gives an overapproximation of the number of boolean variables needed
    uint64 estimateVariables();

    bool propagate();

    bool atFixPoint() const;

    bool finalize();

    /// returns two lists of variables that do not have lower or upper bounds
    void variablesWithoutBounds(std::vector< clingcon::Variable > &lb,
                                std::vector< clingcon::Variable > &ub);

    /// converts some of the aux literals in the VVS's into normal once
    /// numVars must be the biggest+1 boolean variable used in all
    /// clingcon::Literals so far,
    /// sucht that the test l.var() < numVars is true for all "normal" variables
    /// and false for all aux variables
    void convertAuxLiterals(std::vector< const clingcon::VolatileVariableStorage * > &vvs,
                            unsigned int numVars);

    /// a reference to all linear implications
    /// pre: prepare and createClauses must have been called
    std::vector< ReifiedLinearConstraint > &constraints()
    {
        for (auto &i : linearConstraints_)
        {
            assert(i.impl == Direction::FWD);
            (( void )(i));
        }
        return linearConstraints_;
    }

    VariableCreator &getVariableCreator() { return vc_; }
    const VariableCreator &getVariableCreator() const { return vc_; }

    /// pre: l.normalized()
    /// pre: l.getViews().size()<=1
    Literal getLitFromUnary(const LinearConstraint &l)
    {
        assert(l.getViews().size() <= 1);
        assert(l.normalized());
        if (l.getConstViews().size() == 0)
        {
            switch (l.getRelation())
            {
            case LinearConstraint::Relation::EQ:
                return 0 == l.getRhs() ? s_.trueLit() : s_.falseLit();
            case LinearConstraint::Relation::NE:
                return 0 == l.getRhs() ? s_.trueLit() : s_.falseLit();
            case LinearConstraint::Relation::LE:
                return 0 <= l.getRhs() ? s_.trueLit() : s_.falseLit();
            case LinearConstraint::Relation::LT:
                return 0 < l.getRhs() ? s_.trueLit() : s_.falseLit();
            case LinearConstraint::Relation::GT:
                return 0 > l.getRhs() ? s_.trueLit() : s_.falseLit();
            case LinearConstraint::Relation::GE:
                return 0 >= l.getRhs() ? s_.trueLit() : s_.falseLit();
            default:
                assert(false);
            }
        }

        View v = *l.getViews().begin();
        Restrictor r = vc_.getRestrictor(v);

        auto it = clingcon::wrap_lower_bound(r.begin(), r.end(), l.getRhs());
        switch (l.getRelation())
        {
        case LinearConstraint::Relation::EQ:
        {
            it = (it == r.end() || *it != l.getRhs()) ? r.end() : it;
            return vc_.getEqualLit(it);
        }
        case LinearConstraint::Relation::NE:
        {
            it = (it == r.end() || *it != l.getRhs()) ? r.end() : it;
            return ~vc_.getEqualLit(it);
        }
        case LinearConstraint::Relation::LE:
            return vc_.getLELiteral(it);
        case LinearConstraint::Relation::LT:
        case LinearConstraint::Relation::GT:
        case LinearConstraint::Relation::GE:
        default:
            assert(false);
        }
        assert(false);
        return Literal(0);
    }

    Literal getEqualLit(View v, int i)
    {
        LinearConstraint l(LinearConstraint::Relation::EQ);
        l.add(v);
        l.addRhs(i);
        l.normalize();
        return getLitFromUnary(l);
    }

    // private:

    /// pre: prepare()
    bool auxprepare();

    /// uses ReifiedDomainConstraints and ReifiedLinearConstraints to calculate
    /// initial variable domain
    /// can remove elements/reorder domainConstraints_/linearConstraints_
    /// return false if domain gets empty
    bool calculateDomains();
    /// throw exception (currently only std::string) if a domain was not
    /// restricted
    /// on one of the sides

    /// add constraint l as implications to vector insert
    bool convertLinear(ReifiedLinearConstraint &&l, std::vector< ReifiedLinearConstraint > &insert);
    bool addDomainConstraint(ReifiedDomainConstraint &&l);
    bool addDistinct(ReifiedAllDistinct &&l);
    bool addPidgeonConstraint(ReifiedAllDistinct &l);
    bool addPermutationConstraint(ReifiedAllDistinct &l);
    bool addDistinctPairwiseUnequal(ReifiedAllDistinct &&l);
    // bool addDistinctHallIntervals(ReifiedAllDistinct&& l);
    bool addDistinctCardinality(ReifiedAllDistinct &&l);

    void addMinimize();
    /// if constraint is true/false and (0-1 ary), retrict the domain and return
    /// true on first parameter(can be simplified away),
    ///  else false
    /// second parameter is false if domain gets empty or UNSAT
    std::pair< bool, bool > deriveSimpleDomain(ReifiedDomainConstraint &d);
    /// pre: constraint must be normalized
    std::pair< bool, bool > deriveSimpleDomain(const ReifiedLinearConstraint &l);

    void addClause(LitVec v);
    /// exactly one value can be true
    bool createOrderClauses();
    /// mapping of order to direct variables
    bool createEqualClauses();

    /// a list of all constraints
    // std::vector<ReifiedLinearConstraint> linearImplications_;  /// normalized
    // LE implications

    std::vector< ReifiedLinearConstraint > linearConstraints_;
    std::vector< ReifiedAllDistinct > allDistincts_;
    std::vector< ReifiedDomainConstraint > domainConstraints_;
    std::vector< std::pair< View, unsigned int > > minimize_; /// Views on a level to minimize

    Grounder &s_;
    VariableCreator vc_;
    Config conf_;

    std::unique_ptr< LinearPropagator > propagator_;
    bool firstRun_;
    unsigned int varsBefore_;        /// the number of variables that we had before this
                                     /// step
    unsigned int varsAfter_;         /// the highest problem specific variable in this step + 1
    unsigned int varsAfterFinalize_; /// the highest variable we have after this
                                     /// step
};
}
