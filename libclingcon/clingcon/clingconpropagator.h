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
        Literal trueLit, Stats &s, ClingconStats &clingcon_stats, const VariableCreator &vc,
        const Config &conf, const NameList *names,
        const std::vector< ReifiedLinearConstraint > &constraints,
        std::unordered_map< clingcon::Var, std::vector< std::pair< Variable, int32 > > >
            propVar2cspVar,
        const std::vector< bool > &watched)
        : trueLit_(trueLit)
        , stats_(s)
        , clingcon_stats_(clingcon_stats)
        , p_(std::make_unique< LinearLiteralPropagator >(trueLit_, vc, conf))
        , pendingProp_(false)
        , conf_(conf)
        , names_(names)
        , propVar2cspVar_(propVar2cspVar)
        , dls_{0}
        , assertConflict_(false)
        , watched_(watched)
    {
        p_->addLevel();
        p_->addImp(constraints);
        for (size_t i = 0; i < constraints.size(); ++i)
        {
            var2Constraints_[abs(constraints[i].v)].emplace_back(i);
        }
    }

    PropagatorThread(const PropagatorThread &) = delete;
    PropagatorThread(PropagatorThread &&other) = default;


    /// propagator interface
    void init(Clingo::PropagateInit &init);
    void propagate(Clingo::PropagateControl &control, Clingo::LiteralSpan changes);
    void check(Clingo::PropagateControl &control);
    void undo(Clingo::PropagateControl const &s, Clingo::LiteralSpan changes);

    // Solver &solver() { return s_; }
    const VolatileVariableStorage &getVVS() const { return p_->getVVS(); }

    void extend_model(Clingo::Model &m) const;
    void printAssignment() const;
    /// returns range of free values, or 0 if variable is not valid or important
    uint64_t free_range(Variable v, Solver &s) const;
    int32_t value(Variable v, Solver &s) const;

private:
    bool propagateOrderVariables(Clingo::PropagateControl &control, Clingo::LiteralSpan changes);
    bool propagateConstraintVariables(Clingo::PropagateControl &control);
    bool isModel(Clingo::PropagateControl &control);

    /// add a watch for var<=a for iterator it
    /// step is the precalculated number of it-getLiteralRestrictor(var).begin()
    void addWatch(Clingo::PropagateControl &init, const Variable &var, Literal cl, size_t step);
    /// debug functions
    void printPartialState(const Solver &s);
    bool orderLitsAreOK(const Solver &s);

    /// force a new literal l, associated with it to be true,
    /// where l==x>it because x>it+eps
    /// where eps is the next valid literal
    bool forceKnownLiteralLE(ViewIterator it, Literal l, Solver &s);
    bool forceKnownLiteralGE(ViewIterator it, Literal l, Solver &s);

    Literal trueLit_;
    Stats &stats_;
    ClingconStats &clingcon_stats_;
    std::unique_ptr< LinearLiteralPropagator > p_;
    bool pendingProp_; // if there is still unit or other propagation pending and no conflict
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
    ClingconPropagator(Stats &stats, Literal trueLit, const VariableCreator &vc, const Config &conf,
                       NameList *names, SymbolMap *symbols,
                       const std::vector< ReifiedLinearConstraint > &constraints)
        : stats_(stats)
        , trueLit_(trueLit)
        , vc_(vc)
        , conf_(conf)
        , names_(names)
        , symbols_(symbols)
        , constraints_(constraints)
    {
    }
    virtual ~ClingconPropagator() {}

    /// propagator interface
    virtual void init(Clingo::PropagateInit &init) override;
    virtual void propagate(Clingo::PropagateControl &control, Clingo::LiteralSpan changes) override;
    virtual void check(Clingo::PropagateControl &control) override;
    virtual void undo(Clingo::PropagateControl const &s, Clingo::LiteralSpan changes) override;

    void extend_model(Clingo::Model &m)
    {
        auto threadID = m.thread_id();
        assert(threads_.size() > threadID);
        threads_[threadID].extend_model(m);
    }

    Variable lookup(clingo_symbol_t symbol) const
    {
        clingcon::Variable max = 0;
        auto it = symbols_->find(Clingo::Symbol(symbol));
        if (it != symbols_->end()) return it->second.v;
        return static_cast< Variable >(vc_.numVariables());
    }

    size_t num_variables() const { return vc_.numVariables(); }

    Clingo::Symbol symbol(Variable var) const
    {
        for (const auto &i : *symbols_)
        {
            if (i.second == var) return i.first;
        }
        assert(false);
        return Clingo::Symbol();
    }

    bool has_unique_value(size_t thread_id, Variable v) const
    {
        // return threads_[thread_id].free_range(v)==1;
        /// temporarily
        return false;
    }

    int32_t value(size_t thread_id, Variable v) const
    {
        // assert(has_unique_value(thread_id, v));
        // return threads_[thread_id].value(v);
        /// temporarily
        return 0;
    }

private:
    /// add a watch for var<=a for iterator it
    /// step is the precalculated number of it-getLiteralRestrictor(var).begin()
    void addWatch(Clingo::PropagateInit &init, const Variable &var, Literal cl, size_t step);
    Stats &stats_;
    Literal trueLit_;
    const VariableCreator &vc_;
    Config conf_;
    NameList *names_;
    SymbolMap *symbols_;
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
} // namespace clingcon
