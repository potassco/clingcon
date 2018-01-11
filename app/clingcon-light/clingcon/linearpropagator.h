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
#include <algorithm>
#include <cmath>
#include <map>

#include <clingcon/constraint.h>
#include <clingcon/solver.h>
#include <clingcon/storage.h>

namespace clingcon
{

class LinearPropagator;
class LinearLiteralPropagator;

/// weird interface
class ConstraintStorage
{
private:
    ConstraintStorage(const Config &conf)
        : conf_(conf)
    {
    }
    friend LinearPropagator;
    friend LinearLiteralPropagator;
    /// add an implication constraint l.v -> l.l
    void addImp(ReifiedLinearConstraint &&l);
    /// add several implication constraints l.v -> l.l
    void addImp(std::vector< ReifiedLinearConstraint > &&l);
    // add several implication constraints l.v -> l.l
    void addImp(const std::vector< ReifiedLinearConstraint > &l);
    /// remove all constraints,
    /// moves the list of all reified implications out of the object
    std::vector< ReifiedLinearConstraint > removeConstraints();
    void addLevel() { assert(toProcess_.empty()); }
    void removeLevel();
    /// true if we are at a fixpoint
    bool atFixPoint() const { return toProcess_.empty(); }
    /// return false if the domain is empty
    void constrainUpperBound(const View &view, const BaseSolver &s);
    void constrainLowerBound(const View &view, const BaseSolver &s);
    void queueConstraint(std::size_t id);
    std::size_t popConstraint();

private:
    /// a list of all constraints
    std::vector< ReifiedLinearConstraint > linearImpConstraints_;
    std::vector< std::size_t > toProcess_; // a list of constraints that need to be processed
    // a list of constraints that have to be processed if the bound of the
    // variable changes and the constraint is TRUE (opposite case for false, and
    // dont care for unknown)
    std::vector< std::vector< std::size_t > > lbChanges_;
    std::vector< std::vector< std::size_t > > ubChanges_;
    Config conf_;
};

class LinearPropagator
{
public:
    LinearPropagator(Grounder &s, const VariableCreator &vc, const Config &conf)
        : storage_(conf)
        , s_(s)
        , vs_(vc, s.trueLit())
    {
    }

    Grounder &getSolver() { return s_; }

    void addImp(ReifiedLinearConstraint &&l) { storage_.addImp(std::move(l)); }
    /// add several implication constraints l.v -> l.l
    void addImp(std::vector< ReifiedLinearConstraint > &&l) { storage_.addImp(std::move(l)); }
    /// remove all constraints,
    /// moves the list of all reified implications out of the object
    std::vector< ReifiedLinearConstraint > removeConstraints()
    {
        return storage_.removeConstraints();
    }
    void addLevel()
    {
        storage_.addLevel();
        vs_.addLevel();
    }
    void removeLevel()
    {
        storage_.removeLevel();
        vs_.removeLevel();
    }

    /// propagate all added constraints to a fixpoint
    /// return false if a domain gets empty
    /// propagates some literals using a creatingSolver
    bool propagate();

    /// return true if last propagate call did add
    /// nogoods that imply a constraint literal
    bool propagated() const;

    const VariableStorage &getVariableStorage() const { return vs_; }

private:
    /// return false if the domain is empty
    /// iterator u points to the first element not in the domain
    bool constrainUpperBound(const ViewIterator &u);
    /// propagate only a bit (not fixpoint),
    /// return false if a domain gets empty
    bool propagateSingleStep();

    /// computes the min/maximum of the lhs
    std::pair< int64, int64 > computeMinMax(const LinearConstraint &l);

    /// propagates directly, thinks the constraint is true
    /// can result in an empty domain, if so it returns false
    /// can only handle LE constraints
    /// DOES NOT GUARANTEE A FIXPOINT (just not sure)(but reshedules if not)
    /// Remarks: uses double for floor/ceil -> to compatible with 64bit integers
    bool propagate_true(const LinearConstraint &l);

    /// propagates the truthvalue of the constraint if it can be directly
    /// inferred
    /// can only handle LE constraints
    bool propagate_impl(ReifiedLinearConstraint &rl);

private:
    ConstraintStorage storage_;
    Grounder &s_;
    VariableStorage vs_;
    bool propagated_;
};

class LinearLiteralPropagator
{
public:
    using iter = Restrictor::ViewIterator;
    using itervec = std::vector< iter >;
    using LinearConstraintClause = std::pair< Literal, itervec >;

public:
    LinearLiteralPropagator(Solver &s, const VariableCreator &vs, const Config &conf)
        : storage_(conf)
        , s_(s)
        , vs_(vs, s.trueLit())
        , conf_(conf)
    {
    }

    Solver &getSolver() { return s_; }
    VolatileVariableStorage &getVVS() { return vs_; }
    const VolatileVariableStorage &getVVS() const { return vs_; }

    void addImp(ReifiedLinearConstraint &&l) { storage_.addImp(std::move(l)); }
    /// add several implication constraints l.v -> l.l
    void addImp(const std::vector< ReifiedLinearConstraint > &l) { storage_.addImp(l); }
    /// remove all constraints,
    /// moves the list of all reified implications out of the object
    std::vector< ReifiedLinearConstraint > removeConstraints()
    {
        return storage_.removeConstraints();
    }
    const std::vector< ReifiedLinearConstraint > &constraints() const
    {
        return storage_.linearImpConstraints_;
    }
    void addLevel()
    {
        storage_.addLevel();
        vs_.getVariableStorage().addLevel();
    }
    void removeLevel()
    {
        storage_.removeLevel();
        vs_.getVariableStorage().removeLevel();
    }

    /// true if we are at a fixpoint, propagateSingleStep does not do anything
    /// anymore
    bool atFixPoint() { return storage_.atFixPoint(); }
    /// the same but generates a set of reasons
    /// reference is valid until the next call of propagateSingleStep
    std::vector< LinearConstraintClause > &propagateSingleStep();
    /// propagate all added constraints to a fixpoint
    /// return false if a domain gets empty
    bool propagate();

    /// return false if the domain is empty
    /// iterator points to element after the bound
    bool constrainUpperBound(const ViewIterator &u);
    /// return false if the domain is empty
    bool constrainLowerBound(const ViewIterator &l);

    /// add a constraint (identified by id) to the propagation queue
    void queueConstraint(std::size_t id) { storage_.queueConstraint(id); }

    // VariableStorage& getVariableStorage() { return vs_; }

private:
    /// computes the min/maximum of the lhs
    std::pair< int64, int64 > computeMinMax(const LinearConstraint &l, itervec &clause);
    int64 computeMin(const LinearConstraint &l, itervec &clause);

    /// propagates directly, thinks the constraint is true
    /// can result in an empty domain, if so it returns false
    /// can only handle LE constraints
    /// DOES NOT GUARANTEE A FIXPOINT (just not sure)(but reshedules if not)
    /// Remarks: uses double for floor/ceil -> to compatible with 64bit integers
    void propagate_true(const ReifiedLinearConstraint &l);

    /// propagates the truthvalue of the constraint if it can be directly
    /// inferred
    /// can only handle LE constraints
    void propagate_impl(ReifiedLinearConstraint &rl);

private:
    ConstraintStorage storage_;
    Solver &s_;
    VolatileVariableStorage vs_;
    itervec propClause_;
    std::vector< LinearLiteralPropagator::LinearConstraintClause >
        propClauses_; /// temp variable for generatedclauses
    Config conf_;
};
}
