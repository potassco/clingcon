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


/// sign of the literal, for order literals a positive literal is alsways a <= x, while a negative
/// one is a > x
class PropagatorThread
{
public:
    PropagatorThread(
        Solver &s, const VariableCreator &vc, const Config &conf, const NameList *names,
        const std::vector< ReifiedLinearConstraint > &constraints,
        std::unordered_map< clingcon::Var, std::vector< std::pair< Variable, int32 > > >
            propVar2cspVar,
        const std::vector< bool > &watched)
        : s_(s)
        , p_(s, vc, conf)
        , pendingProp_(false)
        , conf_(conf)
        , names_(names)
        , propVar2cspVar_(propVar2cspVar)
        , dls_{0}
        , assertConflict_(false)
        , watched_(watched)
    {
        p_.addLevel();
        p_.addImp(constraints);
        for (size_t i = 0; i < constraints.size(); ++i)
        {
            var2Constraints_[abs(constraints[i].v)].emplace_back(i);
        }
    }

    /// propagator interface
    void init(Clingo::PropagateInit &init);
    void propagate(Clingo::PropagateControl &control, Clingo::LiteralSpan changes);
    void check(Clingo::PropagateControl &control);
    void undo(Clingo::PropagateControl const &s, Clingo::LiteralSpan changes);

    Solver &solver() { return s_; }
    const VolatileVariableStorage &getVVS() const { return p_.getVVS(); }

    void printAssignment() const;

private:
    bool propagateOrderVariables(Clingo::PropagateControl &control, Clingo::LiteralSpan changes);
    bool propagateConstraintVariables(Clingo::PropagateControl &control);
    bool isModel(Clingo::PropagateControl &control);

    /// add a watch for var<=a for iterator it
    /// step is the precalculated number of it-getLiteralRestrictor(var).begin()
    void addWatch(Clingo::PropagateControl &init, const Variable &var, Literal cl,
                  unsigned int step);
    /// debug functions
    void printPartialState();
    bool orderLitsAreOK();

    /// force a new literal l, associated with it to be true,
    /// where l==x>it because x>it+eps
    /// where eps is the next valid literal
    bool forceKnownLiteralLE(ViewIterator it, Literal l, Clingo::PropagateControl &control);
    bool forceKnownLiteralGE(ViewIterator it, Literal l);


    Solver &s_;
    LinearLiteralPropagator p_;
    bool pendingProp_;  // if there is still unit or other propagation pending and no conflict
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
    bool assertConflict_;

    /// Clasp Variable to Constraint ID
    std::unordered_map< Var, std::vector< size_t > > var2Constraints_;
    const std::vector< bool > &watched_;
};


/////
/// \brief The ClingconPropagator class
///  The Base needs to be shared between OrderPropagator and ConstraintPropagator, using the same
///  LinearLiteralPropagator,
/// for each thread
/// The PropagatorThread has to be created in the init phase

class ClingconPropagator : public Clingo::Propagator
{
public:
    ClingconPropagator(Literal trueLit, const VariableCreator &vc, const Config &conf,
                       NameList *names, const std::vector< ReifiedLinearConstraint > &constraints)
        : trueLit_(trueLit)
        , vc_(vc)
        , conf_(conf)
        , names_(names)
        , constraints_(constraints)
    {
    }
    virtual ~ClingconPropagator() {}

    /// propagator interface
    virtual void init(Clingo::PropagateInit &init) override;
    virtual void propagate(Clingo::PropagateControl &control, Clingo::LiteralSpan changes) override;
    virtual void check(Clingo::PropagateControl &control) override;
    virtual void undo(Clingo::PropagateControl const &s, Clingo::LiteralSpan changes) override;

    void printAssignment(size_t threadID)
    {
        assert(threads_.size() > threadID);
        threads_[threadID].printAssignment();
    }

private:
    /// add a watch for var<=a for iterator it
    /// step is the precalculated number of it-getLiteralRestrictor(var).begin()
    void addWatch(Clingo::PropagateInit &init, const Variable &var, Literal cl, unsigned int step);
    Literal trueLit_;
    Solver s_;
    const VariableCreator &vc_;
    Config conf_;
    NameList *names_;
    std::vector< ReifiedLinearConstraint > constraints_;
    std::unordered_map< clingcon::Var, std::vector< std::pair< Variable, int32 > > >
        propVar2cspVar_;

    /// for each Clasp Variable there is a vector of csp Variables with bounds
    /// For each CSP Variable there is an int x
    /// abs(x)-1: steps from the lowest element of the variable to the actual value that is
    /// referenced
    /// sign(x): positive if literal without sign means v <= y, negative if literal with sign means
    /// v <= y
    std::vector< bool > watched_; /// which variables we need to watch
    std::vector< PropagatorThread > threads_;
};
}
