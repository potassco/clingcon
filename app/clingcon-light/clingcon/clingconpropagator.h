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
#include <clingcon/linearpropagator.h>
#include <clingcon/solver.h>
#include <clingcon/theoryparser.h>
#include <cstdint>
#include <memory>
#include <unordered_map>


namespace clingcon
{


/// this will be an shared object between all propagators for the same thread
class PropagatorThreadBase
{
public:
    PropagatorThreadBase(
        Solver &s, const VariableCreator &vc, const Config &conf, const NameList *names,
        const std::vector< ReifiedLinearConstraint > &constraints,
        std::unordered_map< clingcon::Var, std::vector< std::pair< Variable, int32 > > >
            propVar2cspVar)
        : p_(s, vc, conf)
        , conf_(conf)
        , names_(names)
        , propVar2cspVar_(propVar2cspVar)
    {
        p_.addImp(constraints);
    }

    void propagate(Clingo::PropagateControl &control, Clingo::LiteralSpan &changes);
    void undo(Clingo::PropagateControl &s, LiteralSpan changes);
    LinearLiteralPropagator p_;
    Config conf_;
    std::vector< std::string > show_; /// order::Variable -> string name
    std::string outputbuf_;
    /// values of all shown variables in the last model
    std::unordered_map< Variable, int32 > lastModel_;
    /// for every Variable, a name and a disjunction of condition if shown
    const NameList *names_;
    /// for each Clasp Variable there is a vector of csp Variables with bounds
    /// For each CSP Variable there is an int x
    /// abs(x)-1: steps from the lowest element of the variable to the actual value that is
    /// referenced
    /// sign(x): positive if literal without sign means v <= y, negative if literal with sign means
    /// v <= y
    std::unordered_map< Var, std::vector< std::pair< Variable, int32 > > >
        propVar2cspVar_;             /// Clasp Variables to csp variables+bound
    std::vector< std::size_t > dls_; /// every decision level that we are registered for
};

/// sign of the literal, for order literals a positive literal is alsways a <= x, while a negative
/// one is a > x
class ClingconOrderPropagatorThread : public Clingo::Propagator
{
public:
    ClingconOrderPropagatorThread(Solver &s, PropagatorThreadBase &base)
        : s_(s)
        , base_(base)
        , dls_{0}
        , assertConflict_(false)
    {
    }
    virtual ~ClingconOrderPropagatorThread() {}

    /// propagator interface
    virtual void init(Clingo::PropagateInit &init) override;
    virtual void propagate(Clingo::PropagateControl &control,
                           Clingo::LiteralSpan &changes) override;
    virtual void check(Clingo::PropagateControl &control) override;
    virtual void undo(Clingo::PropagateControl &s, LiteralSpan changes) override;

    /// constraint interface
    virtual PropResult propagate(Clasp::Solver &s, Clasp::Literal p, uint32 &data) override;
    virtual void reason(Clasp::Solver &s, Clasp::Literal p, Clasp::LitVec &lits) override;
    virtual void undoLevel(Clasp::Solver &s) override;

    void addLazyShow(Variable v, const std::string &s)
    {
        show_.resize(std::max(( unsigned int )(show_.size()), v + 1));
        show_[v] = s;
    }


    //    const char* printModel(Variable v, const std::string& name);
    //    /// only to be used of a model has been found
    //    bool getValue(Variable v, int32& value);

    Clasp::Solver &solver() { return s_; }


    const VolatileVariableStorage &getVVS() const { return base_.p_.getVVS(); }

private:
    /// add a watch for var<=a for iterator it
    /// step is the precalculated number of it-getLiteralRestrictor(var).begin()
    void addWatch(Clingo::PropagateInit &init, const Variable &var, const Clasp::Literal &cl,
                  unsigned int step);
    /// debug function
    bool orderLitsAreOK();


    /// force a new literal l, associated with it to be true,
    /// where l==x>it because x>it+eps
    /// where eps is the next valid literal
    void forceKnownLiteralLE(ViewIterator it, Literal l);
    void forceKnownLiteralGE(ViewIterator it, Literal l);


    Solver &s_;
    PropagatorThreadBase base_;
    bool assertConflict_;
    /// for every variable i store a reason if i have to give it,
    /// can contain reasons that are no longer valid (does not shrink)
    std::unordered_map< Var, LitVec > reasons_;
    /// only set in imediate conflict in addition to reasons,
    /// as reason can already be set for this variable (opposite sign)
    LitVec conflict_;
};

class ClingconConstraintPropagatorThread : public Clingo::Propagator
{
public:
    ClingconConstraintPropagatorThread(Solver &s, PropagatorThreadBase &base,
                                       const std::vector< ReifiedLinearConstraint > &constraints)
        : s_(s)
        , base_(base)
        , dls_{0}
        , assertConflict_(false)
    {
        base_.p_.addImp(constraints);
        for (size_t i = 0; i < constraints.size(); ++i)
        {
            assert(var2Constraints_.find(abs(constraints[i].v) == var2Constraints_.end()));
            var2Constraints_[abs(constraints[i].v)] = i;
        }
    }
    virtual ~ClingconOrderPropagatorThread() {}

    /// propagator interface
    virtual void init(Clingo::PropagateInit &init) override;
    virtual void propagate(Clingo::PropagateControl &control,
                           Clingo::LiteralSpan &changes) override;
    virtual void check(Clingo::PropagateControl &control) override;
    virtual void undo(Clingo::PropagateControl &s, LiteralSpan changes) override;

    /// constraint interface
    virtual PropResult propagate(Clasp::Solver &s, Clasp::Literal p, uint32 &data) override;
    virtual void reason(Clasp::Solver &s, Clasp::Literal p, Clasp::LitVec &lits) override;
    virtual void undoLevel(Clasp::Solver &s) override;

    /// TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! DESTROY MUSS ÜBERLADEN WERDEN, und watches removed
    ///
    ///
    void addLazyShow(Variable v, const std::string &s)
    {
        show_.resize(std::max(( unsigned int )(show_.size()), v + 1));
        show_[v] = s;
    }


    //    const char* printModel(Variable v, const std::string& name);
    //    /// only to be used of a model has been found
    //    bool getValue(Variable v, int32& value);

    Clasp::Solver &solver() { return s_; }


    const VolatileVariableStorage &getVVS() const { return base_.p_.getVVS(); }

private:
    /// add a watch for var<=a for iterator it
    /// step is the precalculated number of it-getLiteralRestrictor(var).begin()
    void addWatch(Clingo::PropagateInit &init, const Variable &var, const Clasp::Literal &cl,
                  unsigned int step);
    /// debug function
    bool orderLitsAreOK();
    Solver &s_;

    PropagatorThreadBase base_;


    /// force a new literal l, associated with it to be true,
    /// where l==x>it because x>it+eps
    /// where eps is the next valid literal
    void forceKnownLiteralLE(ViewIterator it, Literal l);
    void forceKnownLiteralGE(ViewIterator it, Literal l);

    // const std::vector<std::unique_ptr<LitVec> >& var2OrderLits_; /// CSP variables to
    // Clasp::order

    std::vector< std::size_t > dls_; /// every decision level that we are registered for

    bool assertConflict_;

    /// for every variable i store a reason if i have to give it,
    /// can contain reasons that are no longer valid (does not shrink)
    std::unordered_map< Var, LitVec > reasons_;
    /// only set in imediate conflict in addition to reasons,
    /// as reason can already be set for this variable (opposite sign)
    LitVec conflict_;
    /// Clasp Variable to Constraint ID
    std::unordered_map< Var, size_t > var2Constraints_;
};


/////
/// \brief The ClingconOrderPropagator class
///  The Base needs to be shared between OrderPropagator and ConstraintPropagator, using the same
///  LinearLiteralPropagator,
/// for each thread
/// The Base has to be created in the init phase, outside the actual propagators

class ClingconOrderPropagator : public Clingo::Propagator
{
public:
    ClingconOrderPropagator(std::vector< PropagatorThreadBase > &bases, Solver &s,
                            const VariableCreator &vc, const Config &conf)
        : s_(s)
        , vc_(vc)
        , conf_(conf)
        , constraints_(constraints)
        , assertConflict_(false)
        , bases_(bases)
    {
        for (const auto &i : bases) orderProps_.emplace_back(s, i);
    }
    virtual ~ClingconOrderPropagator() {}

    /// propagator interface
    virtual void init(Clingo::PropagateInit &init) override;
    virtual void propagate(Clingo::PropagateControl &control,
                           Clingo::LiteralSpan &changes) override;
    virtual void check(Clingo::PropagateControl &control) override;
    virtual void undo(Clingo::PropagateControl &s, LiteralSpan changes) override;

private:
    void addWatch(Clingo::PropagateInit &init, const Variable &var, const Literal &cl,
                  unsigned int step);
    Solver &s_;
    const VariableCreator &vc_;
    Config conf_;
    std::vector< ReifiedLinearConstraint > constraints_;
    bool assertConflict_;
    /// for each Clasp Variable there is a vector of csp Variables with bounds
    /// For each CSP Variable there is an int x
    /// abs(x)-1: steps from the lowest element of the variable to the actual value that is
    /// referenced
    /// sign(x): positive if literal without sign means v <= y, negative if literal with sign means
    /// v <= y
    std::vector< bool > watched_; /// which variables we need to watch
    std::vector< PropagatorThreadBase > &bases_;
    std::vector< ClingconOrderPropagatorThread > orderProps_;
};

class ClingconConstraintPropagator : public Clingo::Propagator
{
public:
    ClingconConstraintPropagator(std::vector< PropagatorThreadBase > &bases, Solver &s,
                                 const VariableCreator &vc, const Config &conf)
        : s_(s)
        , vc_(vc)
        , conf_(conf)
        , assertConflict_(false)
    {
        for (const auto &i : bases) constraintProps_.emplace_back(s, i);
    }
    virtual ~ClingconConstraintPropagator() {}

    /// propagator interface
    virtual void init(Clingo::PropagateInit &init) override;
    virtual void propagate(Clingo::PropagateControl &control,
                           Clingo::LiteralSpan &changes) override;
    virtual void check(Clingo::PropagateControl &control) override;
    virtual void undo(Clingo::PropagateControl &s, LiteralSpan changes) override;

private:
    Solver &s_;
    const VariableCreator &vc_;
    Config conf_;
    bool assertConflict_;
    std::vector< PropagatorThreadBase > &bases_;
    std::vector< ClingconConstraintPropagatorThread > constraintProps_;
}
}
