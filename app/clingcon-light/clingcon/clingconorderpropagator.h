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
#include <clasp/constraint.h>
#include <order/linearpropagator.h>
#include <clingcon/solver.h>
#include <order/equality.h>
#include <clingcon/theoryparser.h>
#include <memory>
#include <cstdint>
#include <unordered_map>


/// needed for unordered_map<Clasp::Literal>
namespace std {
template <> struct hash<Clasp::Literal>
{
    std::size_t operator()(const Clasp::Literal & x) const
    {
        return hash<uint32>()(x.rep());
    }
};
}

namespace clingcon
{


///sign of the literal, for order literals a positive literal is alsways a <= x, while a negative one is a > x
class ClingconOrderPropagator : public Clasp::PostPropagator
{
public:

    using DataBlob = Clasp::Literal;

    ClingconOrderPropagator(Clasp::Solver& s, const order::VariableCreator& vc, const order::Config& conf,
                       const std::vector<order::ReifiedLinearConstraint>& constraints, const order::EqualityProcessor::EqualityClassMap& equalities,
                            const NameList* names) :
        s_(s), conf_(conf), ms_(new MyLocalSolver(s)), p_(*(ms_.get()), vc, conf), eqs_(equalities), dls_{0}, assertConflict_(false), names_(names)
    {
        if (s_.hasConflict())
            return;
        /// access the reified literals
        /// and the order literals, and watch them using
        /// s_.addWatch(Literal(), Constraint(this), data);
        /// then this->propagate(Literal, data, solver s) is called whenever there is a change

        /// if an orderLiteral changes, i change lower/upper bound p_.changeUpperBound(variable, bound_iterator)
        /// Given some Literal + datablob i need to know:
        /// 1. it is an orderLiteral    -> data.bit 1 (sign=true)
        /// 2. the variables with bounds-> lookup Clasp::Var -> [Constraint::var1+abs(bound1)-1,var2+abs(bound2)-1, etc...]
        /// 3. the sign                 -> sign of the literal * sign(bound),

        ///
        /// if a reification literal is changed, i have to notify p_ and enable/disable a constraint
        /// I just have to shedule this constraint (it is true)
        /// I need to know
        /// 1. is the literal a reification literal             -> (data.bit 1) (sign=false)
        /// 2. the constraint index                             -> data.var
        /// I just need the clasp variable to see which index has to be sheduled for propagation

        ///
        /// Watch out: A literal can refer to many variables + bounds
        /// Solution: Register several of them, same literal with different vars
        ///
        /// Restricting a certain variable is always exactly one literal (which can be equal to others but we dont care)
        ///
        ///

        watched_.resize(p_.getVVS().getVariableStorage().numVariables(),false);

        /// add watches for undecided reification literals
        for (std::size_t cindex = 0; cindex < constraints.size(); ++cindex)
        {
            for (auto view : constraints[cindex].l.getConstViews())
            {
                watched_[view.v]=true;
            }
            /// just watch the nonfalse ones
            Clasp::Literal l = toClaspFormat(constraints[cindex].v);
            if (!s.isFalse(l)) /// permanent false otherwise, do not need to consider it
            {
                DataBlob blob(cindex,false);
                s.addWatch(l, this, blob.rep());
                //s.addWatch(~l, this, blob.asUint()); // i'm just watching implications
            }
        }


        /// add watches for the order literals
        for (std::size_t var = 0; var != vc.numVariables(); ++var)
        {
            if (vc.isValid(var))
            {
                if (!conf.explicitBinaryOrderClausesIfPossible)
                    watched_[var]=true;
                else
                if (p_.getVVS().getVariableStorage().getOrderStorage(var).numLits() < p_.getVVS().getVariableStorage().getDomain(var).size())
                    watched_[var]=true;
            }
        }

        for (std::size_t var = 0; var != watched_.size(); ++var)
        {
            if (watched_[var])
            {
                auto lr = order::pure_LELiteral_iterator(vc.getRestrictor(order::View(var)).begin(), p_.getVVS().getVariableStorage().getOrderStorage(var), true);

                while(lr.isValid())
                {
                    ///TODO: do i need to do this for true lits ?
                    addWatch((order::Variable)(var),toClaspFormat(*lr),lr.numElement());
                    ++lr;
                }
            }
        }

        p_.addImp(constraints);
    }
    virtual ~ClingconOrderPropagator()
    {
        for (std::size_t cindex = 0; cindex < p_.constraints().size(); ++cindex)
        {
            if (s_.hasWatch(toClaspFormat(p_.constraints()[cindex].v),this))
            {
                s_.removeWatch(toClaspFormat(p_.constraints()[cindex].v),this);
            }
        }

        for (std::size_t var = 0; var != watched_.size(); ++var)
        {
            if (watched_[var])
            {
                auto lr = order::pure_LELiteral_iterator(p_.getVVS().getVariableStorage().getRestrictor(order::View(var)).begin(), p_.getVVS().getVariableStorage().getOrderStorage(var), true);
                while(lr.isValid())
                {
                    s_.removeWatch(toClaspFormat(*lr), this);
                    s_.removeWatch(toClaspFormat(~(*lr)), this);
                    ++lr;
                }
            }


        }
    }

    /// propagator interface
    virtual uint32 priority() const override { return Clasp::PostPropagator::priority_reserved_ufs+1; } // we schedule after the ufs checker
    virtual bool   init(Clasp::Solver &s) override;
    virtual bool   propagateFixpoint(Clasp::Solver& , Clasp::PostPropagator* ) override;
    virtual void   reset() override;
    virtual bool   isModel(Clasp::Solver& s) override;

    /// constraint interface
    virtual PropResult propagate(Clasp::Solver& s, Clasp::Literal p, uint32& data) override;
    virtual void reason(Clasp::Solver& s, Clasp::Literal p, Clasp::LitVec& lits) override;
    virtual void undoLevel(Clasp::Solver& s) override;
    virtual bool simplify(Clasp::Solver& , bool ) override { return false; }

    ///TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! DESTROY MUSS ÜBERLADEN WERDEN, und watches removed
    ///
    ///
    void addLazyShow(order::Variable v, const std::string& s) { show_.resize(std::max((unsigned int)(show_.size()),v+1)); show_[v]=s; }

    //! Returns an estimate of the constraint's complexity relative to a clause (complexity = 1).
    /// currently not used in clasp propagators
    ///virtual uint32 estimateComplexity(const Clasp::Solver& s) const { return 42; /* do some rought guessing by the number of constraints/size*/}


    const char* printModel(order::Variable v, const std::string& name);
    /// only to be used of a model has been found
    bool getValue(order::Variable v, int32& value);

    Clasp::Solver& solver() { return s_; }


    const order::VolatileVariableStorage& getVVS() const { return p_.getVVS(); }

private:
    /// add a watch for var<=a for iterator it
    /// step is the precalculated number of it-getLiteralRestrictor(var).begin()
    void addWatch(const order::Variable& var, const Clasp::Literal &cl, unsigned int step);
    ///debug function
    bool orderLitsAreOK();
    Clasp::Solver& s_;
    order::Config conf_;
    std::unique_ptr<order::IncrementalSolver> ms_;
    order::LinearLiteralPropagator p_;
    const order::EqualityProcessor::EqualityClassMap& eqs_; /// equalities of variables for printing



    /// force a new literal l, associated with it to be true,
    /// where l==x>it because x>it+eps
    /// where eps is the next valid literal
    void forceKnownLiteralLE(order::ViewIterator it, Clasp::Literal l);
    void forceKnownLiteralGE(order::ViewIterator it, Clasp::Literal l);

    /// for each Clasp Variable there is a vector of csp Variables with bounds
    /// For each CSP Variable there is an int x
    /// abs(x)-1: steps from the lowest element of the variable to the actual value that is referenced
    /// sign(x): positive if literal without sign means v <= y, negative if literal with sign means v <= y
    std::unordered_map<Clasp::Var, std::vector<std::pair<order::Variable, int32> > > propVar2cspVar_;/// Clasp Literals to csp variables+bound
    //const std::vector<std::unique_ptr<order::LitVec> >& var2OrderLits_; /// CSP variables to Clasp::order

    std::vector<std::size_t> dls_; /// every decision level that we are registered for

    bool assertConflict_;

    std::unordered_map<Clasp::Var, Clasp::LitVec> reasons_; /// for every variable i store a reason if i have to give it,
                                                            /// can contain reasons that are no longer valid (does not shrink)
    Clasp::LitVec conflict_;                               /// only set in imediate conflict in addition to reasons,
                                                            /// as reason can already be set for this variable (opposite sign)

    std::vector<std::string> show_; /// order::Variable -> string name
    std::string outputbuf_;
    std::vector<bool> watched_; /// which variables we need to watch

    std::unordered_map<order::Variable,int32> lastModel_; /// values of all shown variables in the last model
    const NameList* names_; /// for every Variable, a name and a disjunction of condition if shown


    int watchcounter_;
};


}
