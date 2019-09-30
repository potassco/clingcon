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

#include <clingcon/clingconpropagator.h>
#include <clingcon/variable.h>


namespace clingcon
{

void ClingconPropagator::init(Clingo::PropagateInit &init)
{
    /// access the reified literals
    /// and the order literals, and watch them using
    /// s_.addWatch(Literal(), Constraint(this), data);
    /// then this->propagate(Literal, data, solver s) is called whenever there is a change

    /// if an orderLiteral changes, i change lower/upper bound p_->changeUpperBound(variable,
    /// bound_iterator)
    /// Given some Literal + datablob i need to know:
    /// 1. it is an orderLiteral    -> data.bit 1 (sign=true)
    /// 2. the variables with bounds-> lookup Clasp::Var ->
    /// [Constraint::var1+abs(bound1)-1,var2+abs(bound2)-1, etc...]
    /// 3. the sign                 -> sign of the literal * sign(bound),
    ///
    /// Watch out: A literal can refer to many variables + bounds
    /// Solution: Register several of them, same literal with different vars
    ///
    /// Restricting a certain variable is always exactly one literal (which can be equal to
    /// others but we dont care)
    ///
    /// if a reification literal is changed, i have to notify p_ and enable/disable a constraint
    /// I just have to shedule this constraint (it is true)
    /// I need to know
    /// 1. is the literal a reification literal             -> (data.bit 1) (sign=false)
    /// 2. the constraint index                             -> data.var
    /// I just need the clasp variable to see which index has to be sheduled for propagation
    ///
    ///
    stats_.clingcon_stats.resize(init.number_of_threads());
    Timer tinit(stats_.time_init);
    init.set_check_mode(Clingo::PropagatorCheckMode::Partial);

    /// convert aspif literals to solver literals
    trueLit_ = init.solver_literal(trueLit_);
    vc_.convertLiterals(init);
    for (auto &i : constraints_) i.v = init.solver_literal(i.v);
    stats_.num_constraints = constraints_.size();
    if (names_)
        for (auto &i : (*names_))
            for (auto &j : i.second.second) j = init.solver_literal(j);


    /// add watches for undecided reification literals
    for (std::size_t cindex = 0; cindex < constraints_.size(); ++cindex)
    {
        /// just watch the nonfalse ones
        Literal l = constraints_[cindex].v;
        /// cant check truth value, as solver needs control object, not init
        // if (!s_.isFalse(l)) /// permanent false otherwise, do not need to consider it
        {
            // std::cout << "ConstraintProp watch " << l << std::endl;
            init.add_watch(l);
            // for (auto const& cpthread : constraintProps_)
            //    cpthread.addWatch(init, l);
            // s.addWatch(-l, this, blob.asUint()); // i'm just watching implications
        }
    }

    watched_.resize(vc_.numVariables(), false);
    for (std::size_t cindex = 0; cindex < constraints_.size(); ++cindex)
    {
        for (auto view : constraints_[cindex].l.getConstViews())
        {
            watched_[view.v] = true;
        }
    }

    /// add watches for the order literals
    for (std::size_t var = 0; var != vc_.numVariables(); ++var)
    {
        if (vc_.isValid(Variable(var)))
        {
            // if (!conf_.explicitBinaryOrderClausesIfPossible)
            watched_[var] = true;
            // slse if (vc_.numOrderLits(var) < vc_.getDomain(var).size())
            //    watched_[var] = true;
        }
    }
    stats_.num_int_variables = vc_.numVariables();

    for (std::size_t var = 0; var != watched_.size(); ++var)
    {
        if (watched_[var])
        {
            auto lr = pure_LELiteral_iterator(vc_.getRestrictor(View(Variable(var))).begin(),
                                              vc_.getStorage(Variable(var)), true);

            while (lr.isValid())
            {
                /// TODO: do i need to do this for true lits ?
                addWatch(init, Variable(var), *lr, lr.numElement());
                ++lr;
            }
        }
    }

    for (size_t i = 0; i < init.number_of_threads(); ++i)
        threads_.emplace_back(trueLit_, stats_, stats_.clingcon_stats[i], vc_, conf_, names_,
                              constraints_, propVar2cspVar_, watched_);

    propVar2cspVar_.clear();
    constraints_.clear();
}


void ClingconPropagator::propagate(Clingo::PropagateControl &control, Clingo::LiteralSpan changes)
{
    //std::cout << "called propagate on level " << control.assignment().decision_level() << std::endl;
    assert(control.thread_id() < threads_.size());
    Timer tprop(stats_.clingcon_stats[control.thread_id()].time_propagate);
    threads_[control.thread_id()].propagate(control, changes);
}

void ClingconPropagator::check(Clingo::PropagateControl &control)
{
    //std::cout << "called check on level " << control.assignment().decision_level() << std::endl;
    assert(control.thread_id() < threads_.size());
    Timer tprop(stats_.clingcon_stats[control.thread_id()].time_propagate);
    threads_[control.thread_id()].check(control);
}


void PropagatorThread::propagate(Clingo::PropagateControl &control, Clingo::LiteralSpan changes)
{
    Solver s(stats_, trueLit_, control);
    /// only called if p is gets true (otherwise -p gets true)
    for (auto i : changes)
    {
        assert(s.isTrue(i));
        assert(control.assignment().level(i) == control.assignment().decision_level());
    }

    if (dls_.back() != control.assignment().decision_level())
    {
         //std::cout << "new level " << control.assignment().decision_level() << std::endl;

        dls_.emplace_back(control.assignment().decision_level());
        assert(p_->atFixPoint());
        p_->addLevel();
        // s_.addUndoWatch(control.assignment().decision_level(), this);
        // std::cout << "Variable storage before getting to the next level " <<
        // p_->getVVS().getVariableStorage() << std::endl;
    }
    // std::cout << "assignment size " << control.assignment().size() << "including " <<
    // changes.size() << " changes" << std::endl;
    propagateOrderVariables(control, changes);
}

void PropagatorThread::check(Clingo::PropagateControl &control)
{
    Solver s(stats_, trueLit_, control);
    assert(orderLitsAreOK(s));
    if (!propagateConstraintVariables(control)) return;
    if (!pendingProp_ && control.assignment().is_total()) isModel(control);
}


void PropagatorThread::printPartialState(const Solver &s)
{
    auto &vs = p_->getVVS().getVariableStorage();
    for (Variable i = 0; i < vs.numVariables(); ++i)
    {
        auto r = vs.getCurrentRestrictor(i);
        std::cout << "var" << i << "\t= " << r.lower() << "\t.. " << r.upper() << std::endl;
        if (!vs.hasLELiteral(r.end() - 1))
            std::cout << "upper bound has no LE literal" << std::endl;
        else if (!s.isTrue(vs.getLELiteral(r.end() - 1)))
            std::cout << "upper bound literal is not true" << std::endl;
        if (!vs.hasGELiteral(r.begin()))
            std::cout << "lower bound has no GE literal" << std::endl;
        else if (!s.isTrue(vs.getGELiteral(r.begin())))
            std::cout << "lower bound GE literal is not true" << std::endl;
    }
}


bool PropagatorThread::propagateOrderVariables(Clingo::PropagateControl &control,
                                               Clingo::LiteralSpan changes)
{
    Solver s(stats_, trueLit_, control);
    //printPartialState(s);
    for (auto p : changes)
    {
        //std::cout << " one change in propagate order variables" << std::endl;
        auto constraints = var2Constraints_.find(abs(p));
        if (constraints != var2Constraints_.end())
            for (auto c : constraints->second) p_->queueConstraint(c);
        /// order literal
        auto found = propVar2cspVar_.find(abs(p));
        if (found == propVar2cspVar_.end()) continue;
        const auto &cspVars = found->second;

        for (const auto &cspVar : cspVars)
        {
            if (!propagateVariableAssignment(control, p, cspVar.first, cspVar.second)) return false;
        }
    }

    if (!control.propagate())
    {
        return false;
    }
    else
    {
        assert(!assertConflict_);
        return true;
    }
}

bool PropagatorThread::propagateVariableAssignment(Clingo::PropagateControl &control, Literal p,
                                                   Variable v, int32 value)
{
    Solver s(stats_, trueLit_, control);
    //printPartialState();
    uint64 bound = abs(value) - 1;
    bool sign = value < 0;
//    std::cout << "Propagate order Literal" << std::endl;
//    std::cout << "Variable: " << v << " with bound/sign/p,sign" << bound << "/" << sign << "/"
//              << ((p < 0) ? -1 : 1) << " on dl" << control.assignment().decision_level()
//              << std::endl;
//    std::cout << "Literal: " << p << " @" << control.assignment().decision_level() << std::endl;

    Restrictor lr =
        p_->getVVS().getVariableStorage().getRestrictor(v); //(*orderLits_[cspVar.first]);
    /// these two assertions do not work for incremental
    /// assert(p_->getVVS().getVariableStorage().getDomain(cspVar.first).size() > bound);
    /// /// assure that the given bound is in range
    /// assert(toClaspFormat(p_->getVVS().getVariableStorage().getLELiteral(lr.begin()+bound)).var()
    /// == p.var()); /// given the cspvar+bound we can reinfer the literal which should be p
    /// again

    if ((p < 0 && !sign) || (p > 0 && sign))
    { /// v > bound
        //std::cout << "new lower bound " << *(lr.begin() + bound + 1) << std::endl;
        if (p_->getVVS().getVariableStorage().getCurrentRestrictor(v).begin() <
            (lr.begin() + bound + 1))
        {
            bool nonempty =
                p_->constrainLowerBound(lr.begin() + bound + 1,
                                        s); /// we got not (a<=x) ->the new lower bound is x+1
                                            //                    printPartialState();
            // assert(nonempty); // can happen that variables are propagated in a way that
            // clasp is not yet aware of the conflict, but should find it during this unit
            // propagation
            if (!nonempty)
            {
                assertConflict_ = true;
            }
        }

        /// the start of the free range was (probably) shifted to the right
        /// if range was restricted from 5...100 to 10..100, we make 8,7,6,5 false with
        /// reason x>9
        // if (conf_.minLitsPerVar >= 0 || !conf_.explicitBinaryOrderClausesIfPossible) ///
        // if i precreate all variables and do have explicitBinaryorderClauses, then i do
        // not need to do this
        //                if (/*!base_.conf_.explicitBinaryOrderClausesIfPossible ||*/
        //                        true ||
        //                    (conf_.minLitsPerVar >= 0 &&
        //                     (uint64)(conf_.minLitsPerVar) <
        //                         p_->getVVS().getVariableStorage().getDomain(cspVar.first).size()))
        //                we do not need to do this if
        //                        we create explicit
        //                        and
        //                        either
        //                        minLits = -1
        //                        or
        //                        minLits > domainsize

        if (!((false) && (conf_.minLitsPerVar == -1 ||
                          (uint64)(conf_.minLitsPerVar) <
                              p_->getVVS().getVariableStorage().getDomain(v).size())))
        {
            pure_LELiteral_iterator newit(
                lr.begin() + bound, p_->getVVS().getVariableStorage().getOrderStorage(v), false);
            while ((--newit).isValid())
            {
                if (s.isFalse(*newit)) break;
                /// TODO:s_.addClause();
                if (!s.addClause(LitVec{-(*newit), -p}))
                {
                    return false;
                }
                // if (!s_.force(-(*newit), Clasp::Antecedent(p)))
                //    return PropResult(false, true);
            }
        }
    }
    else
    { /// v <= bound
        //std::cout << "new upper bound " << *(lr.begin() + bound) << std::endl;
        if (p_->getVVS().getVariableStorage().getCurrentRestrictor(v).end() > (lr.begin() + bound))
        {
            bool nonempty =
                p_->constrainUpperBound(lr.begin() + bound + 1,
                                        s); /// we got a <= x, the new end restrictor is at x+1
            // printPartialState();
            // assert(nonempty);
            if (!nonempty)
            { /*std::cout << "now" << std::endl;*/
                assertConflict_ = true;
            }
        }
        /// the end of the free range was (probably) shifted to the left
        /// if range was restricted from 5...100 to 5..95, we make 96,97,98,99 true with
        /// reason x<95

        if (!((false) && (conf_.minLitsPerVar == -1 ||
                          (uint64)(conf_.minLitsPerVar) <
                              p_->getVVS().getVariableStorage().getDomain(v).size())))
        {
            pure_LELiteral_iterator newit(
                lr.begin() + bound, p_->getVVS().getVariableStorage().getOrderStorage(v), true);
            while ((++newit).isValid())
            {
                if (s.isTrue(*newit)) break;
                if (!s.addClause(LitVec{*newit, -p}))
                {
                    return false;
                }
                // if (!s_.force(toClaspFormat(*newit), Clasp::Antecedent(p)))
                //    return PropResult(false, true);
            }
        }
    }
    return true;
}

bool PropagatorThread::propagateConstraintVariables(Clingo::PropagateControl &control)
{
    assert(!assertConflict_);
    Solver s(stats_, trueLit_, control);
    assert(orderLitsAreOK(s));
    pendingProp_ = false;
    while (!p_->atFixPoint())
    {
        const auto &clauses = p_->propagateSingleStep(s);
        //printPartialState(s);
        //std::cout << "propagateSingleStep end" << std::endl;
        auto &vs = p_->getVVS();
        if (clauses.size())
        {
            for (const auto &clause : clauses)
            {
                LitVec claspClause;
                claspClause.push_back(clause.first);
                //std::cout << "clasClause contains reified literal " << clause.first << std::endl;

                const auto &its = clause.second;

                for (unsigned int i = 0; i < its.size(); ++i)
                {
//                                        std::cout << " and " << its[i].view().a << "*v" <<
//                                        its[i].view().v << ">=" << *(its[i])  << " order literal ";
                    if (!vs.getVariableStorage().hasGELiteral(its[i]))
                    {
                                                //std::cout << " does not have one, ";
                        Literal l = s.trueLit();
                        if (control.assignment().decision_level() == 0)
                        {
                            auto current = vs.getVariableStorage().getCurrentRestrictor(its[i].view());
                                if (its[i] > current.end())
                                    l = s.trueLit();
                                else
                                if (its[i] < current.begin())
                                    l = s.falseLit();
                        
                        }
                        else
                        {
                            l = s.getNewLiteral();
                        }
                                                //std::cout << l << std::endl;
                        auto it = its[i] - 1;
//                                                std::cout << " but " << it.view().a << "*v" <<
//                                                it.view().v << "<=" << *it << " gets " << l <<
//                                                std::endl;
                        vs.setLELit(it, l);
                        // claspClause.push_back(toClaspFormat(l));

                        if (it.view().reversed())
                        {
                            auto varit = ViewIterator::viewToVarIterator(it);
                            addWatch(control, varit.view().v, -l, varit.numElement() - 1);
//                            int32 x = -l < 0 ?
//                                          static_cast< int32 >(varit.numElement() - 1 + 1) * -1 :
//                                          static_cast< int32 >(varit.numElement() - 1 + 1);
//                            if (!propagateVariableAssignment(control, l, varit.view().v, x))
//                                return false;
                        }
                        else
                        {
                            auto varit = ViewIterator::viewToVarIterator(it);
                            addWatch(control, varit.view().v, l, varit.numElement());
//                            int32 x = l < 0 ? static_cast< int32 >(varit.numElement() + 1) * -1 :
//                                              static_cast< int32 >(varit.numElement() + 1);
//                            if (!propagateVariableAssignment(control, l, varit.view().v, x))
//                                return false;
                        }
                    }

                    /// now it has a literal
                    {
                        auto currentIt =
                            p_->getVVS().getVariableStorage().getCurrentRestrictor(its[i].view());
                        Literal l = vs.getVariableStorage().getGELiteral(its[i]);
                                                //std::cout << l << std::endl;
                        if (s.isUnknown(l))
                        {
                            auto it = its[i] - 1;
                            if (it < currentIt.begin())
                            {
                                if (it.view().reversed())
                                {
                                    // std::cout << "assignment size " <<
                                    // control.assignment().size() << "/" <<
                                    //          control.assignment().max_size() << " is fixed=" <<
                                    //          control.assignment().is_total() << std::endl;
                                    if (!forceKnownLiteralLE(it, l, s)) return false;
                                }
                                else
                                {
                                    if (!forceKnownLiteralGE(it, l, s)) return false;
                                }
                            }
                            else if (currentIt.end() <= it)
                            {
                                if (it.view().reversed())
                                {
                                    if (!forceKnownLiteralGE(it, l, s)) return false;
                                }
                                else
                                {
                                    if (!forceKnownLiteralLE(it, l, s)) return false;
                                }
                            }
                            /// else it simply free and unit asserting
                        }

                        claspClause.push_back(-l);
                    }
                }

                ////1=true, 2=false, 0=free
                /// all should be isFalse(i)==true, one can be unknown or false
//                std::cout << "Give clasp the clause " << std::endl;
//                for (auto i : claspClause)
//                    std::cout << i << "@" << control.assignment().level(i) << "isFalse?"
//                              << s.isFalse(i) << " isTrue" << s.isTrue(i) << ",   ";
//                std::cout << " on level " << control.assignment().decision_level();
//                std::cout << std::endl;
                //                assert(std::count_if(claspClause.begin(), claspClause.end(),
                //                [&](Literal i) {
                //                           return s_.isFalse(i);
                //                       }) >= claspClause.size() - 1);
                //                assert(std::count_if(claspClause.begin(), claspClause.end(),
                //                [&](Literal i) {
                //                           return s_.isFalse(i) && (control.assignment().level(i)
                //                           ==
                //                                                    control.assignment().decision_level());
                //                       }) >= 1);

                if (!s.addClause(claspClause))
                {
                    return false;
                }
            }
        }
        size_t oldsize = control.assignment().size();
        if (!control.propagate()) return false;
        if (oldsize < control.assignment().size())
        {
            pendingProp_ = true;
            return true; /// do other propagation first
        }
    }
    return true; // control.propagate();
}


void ClingconPropagator::undo(Clingo::PropagateControl const &control, Clingo::LiteralSpan changes)
{
    assert(control.thread_id() < threads_.size());
    Timer tundo(stats_.clingcon_stats[control.thread_id()].time_undo);
    threads_[control.thread_id()].undo(control, changes);
}

void PropagatorThread::undo(Clingo::PropagateControl const &control, Clingo::LiteralSpan)
{
    //std::cout << "reset on dl " << control.assignment().decision_level() << std::endl;
    //        if (control.assignment().decision_level() != 0 &&
    //        control.assignment().decision_level() == base_.dls_.back())
    //        {
    //            base_.p_->removeLevel();
    //            base_.p_->addLevel();
    //        }
    //        else
    assertConflict_ = false;
    while (control.assignment().decision_level() <= dls_.back() && dls_.size() > 1)
    {
        p_->removeLevel();
        dls_.pop_back();
    }
}


bool PropagatorThread::forceKnownLiteralGE(ViewIterator it, Literal l, Solver &s)
{
    // assert(iterator is variterator)
    auto varit = ViewIterator::viewToVarIterator(it);
    pure_LELiteral_iterator pit(
        varit, p_->getVVS().getVariableStorage().getOrderStorage(varit.view().v), 1);
    assert(pit.isValid());
    ++pit;
    assert(pit.isValid());
    while (!s.isFalse(*pit))
    {
        assert(!s.isTrue(*pit));
        ++pit;
        assert(pit.isValid());
    }
    // s_.force(l, Clasp::Antecedent(-(toClaspFormat(*pit))));
    // std::cout << s_.isTrue(l) << " " << s_.isFalse(l) << std::endl;
    return s.addClause({l, *pit}); /// can fail if *pit was assigned on a former level
}

bool PropagatorThread::forceKnownLiteralLE(ViewIterator it, Literal l, Solver &s)
{
    // std::cout << "force Known Literal LE" << std::endl;
    auto varit = ViewIterator::viewToVarIterator(it);
    pure_LELiteral_iterator pit(
        varit, p_->getVVS().getVariableStorage().getOrderStorage(varit.view().v), 0);
    assert(pit.isValid());
    --pit;
    assert(pit.isValid());
    while (!s.isTrue(*pit))
    {
        assert(!s.isFalse(*pit));
        --pit;
        assert(pit.isValid());
    }
    // std::cout << "clause {" << l << "=" << s_.isTrue(l) << "/" << s_.isFalse(l) << " , " <<
    // -(*pit) << "=" << s_.isTrue(-(*pit))<< "/" << s_.isFalse(-(*pit)) << "@" <<
    // control.assignment().level(-(*pit)) <<  "}" << std::endl;
    // s_.force(l, Clasp::Antecedent(toClaspFormat(*pit)));
    return s.addClause({l, -(*pit)}); /// can fail if *pit was assigned on a former level
}


void ClingconPropagator::addWatch(Clingo::PropagateInit &init, const Variable &var, Literal cl,
                                  size_t step)
{
    // std::cout << "OrderProp watch " << cl << " and " << -cl << std::endl;
    init.add_watch(cl);
    init.add_watch(-cl);
    int32 x = (cl < 0) ? static_cast< int32 >(step + 1) * -1 : static_cast< int32 >(step + 1);
    propVar2cspVar_[abs(cl)].emplace_back(std::make_pair(var, x));
}

void PropagatorThread::addWatch(Clingo::PropagateControl &control, const Variable &var, Literal cl,
                                size_t step)
{
    control.add_watch(cl);
    control.add_watch(-cl);
    int32 x = cl < 0 ? static_cast< int32 >(step + 1) * -1 : static_cast< int32 >(step + 1);
    propVar2cspVar_[abs(cl)].emplace_back(std::make_pair(var, x));
}


/// debug function to check if stored domain restrictions are in line with order literal assignment
/// in clasp solver
/// furthermore check if current bound has literals (upper and lower must have lits)
/// THIS CONDITION IS NO LONGER TRUE,
/// I CAN HAVE UNDECIDED VARIABLES OUTSIDE OT THE BOUNDS
/// THEY WILL BE SET IF NEEDED --- really? how
bool PropagatorThread::orderLitsAreOK(const Solver &s)
{
#ifndef NDEBUG
    //printPartialState(s);
    for (Variable var = 0; var < p_->getVVS().getVariableStorage().numVariables(); ++var)
    {
        if (watched_[var] && p_->getVVS().getVariableStorage().isValid(var))
        {
            // std::cout << "check var " << var << std::endl;
            auto currentlr = p_->getVVS().getVariableStorage().getCurrentRestrictor(var);
            auto baselr = p_->getVVS().getVariableStorage().getRestrictor(var);
            assert(!currentlr.isEmpty());

            // std::cout << "Orig domain " << baselr.lower() << ".." << baselr.upper() << " is "
            //          << currentlr.lower() << ".." << currentlr.upper() << std::endl;

            pure_LELiteral_iterator it(
                baselr.begin(), p_->getVVS().getVariableStorage().getOrderStorage(var), true);

            if (!it.isValid()) assert(false);
            while (it.isValid())
            {
                // std::cout << "checking element " << it.numElement() << " True/False:" <<
                // s_.isTrue(*it) << "/" << s_.isFalse(*it) << std::endl; std::cout << "This is
                // value: " << baselr.lower() + it.numElement() << std::endl;
                if (s.isFalse(*it) && it.numElement() >= currentlr.begin().numElement())
                    assert(false);
                if (s.isTrue(*it) && it.numElement() < currentlr.end().numElement() - 1)
                    assert(false);
                if (!s.isFalse(*it) && !s.isTrue(*it) &&
                    (it.numElement() < currentlr.begin().numElement() ||
                     it.numElement() >= currentlr.end().numElement()))
                {
                    // std::cout << "checking literal " << (*it) << " mit elementindex " <<
                    // it.numElement() << std::endl;
                    // std::cout << "this is v" << var << "<=" << *(baselr.begin()+it.numElement())
                    //<< std::endl;
                    // std::cout << "Orig domain " << baselr.lower() << ".." << baselr.upper() <<
                    //             "is " << currentlr.lower() << ".." << currentlr.upper() <<
                    //             std::endl;
                    // assert(false);
                }
                ++it;
            }
        }
    }

#endif
    return true;
}


namespace
{

    /// finds first literal that is not false
    template < class ForwardIt >
    ForwardIt my_lower_bound(ForwardIt first, ForwardIt last, Solver &s, const orderStorage &os)
    {
        ForwardIt it;
        typename std::iterator_traits< ForwardIt >::difference_type count, step;
        count = std::distance(first, last);

        while (count > 0)
        {
            it = first;
            step = count / 2;
            std::advance(it, step);
            if (s.isFalse(os.getLiteral(it.numElement())))
            { /// if (lit is false)
                first = ++it;
                count -= step + 1;
            }
            else
                count = step;
        }
        return first;
    }

    /// finds first literal that is true
    template < class ForwardIt >
    ForwardIt my_upper_bound(ForwardIt first, ForwardIt last, Solver &s, const orderStorage &os)
    {
        ForwardIt it;
        typename std::iterator_traits< ForwardIt >::difference_type count, step;
        count = std::distance(first, last);

        while (count > 0)
        {
            it = first;
            step = count / 2;
            std::advance(it, step);
            if (!s.isTrue(os.getLiteral(it.numElement())))
            { /// if (lit is not true)
                first = ++it;
                count -= step + 1;
            }
            else
                count = step;
        }
        return first;
    }
} // namespace

uint64_t PropagatorThread::free_range(Variable v, Solver &s) const
{
    assert(p_->getVVS().getVariableStorage().isValid(v));
    if (watched_[v])
        return p_->getVVS().getVariableStorage().getCurrentRestrictor(v).size();
    else
    {
        auto rs = p_->getVVS().getVariableStorage().getRestrictor(View(v));
        return static_cast< uint64_t >(
            my_upper_bound(rs.begin(), rs.end(), s,
                           p_->getVVS().getVariableStorage().getOrderStorage(v)) -
            my_lower_bound(rs.begin(), rs.end(), s,
                           p_->getVVS().getVariableStorage().getOrderStorage(v)));
    }
}

int32_t PropagatorThread::value(Variable v, Solver &s) const
{
    ViewIterator vit;
    if (watched_[v])
    {
        Restrictor lr;
        lr = p_->getVVS().getVariableStorage().getCurrentRestrictor(v);
        assert(lr.size() == static_cast< size_t >(1));
        return static_cast< int32_t >(*lr.begin());
    }
    else /// not watched, need to search for value
    {
        auto rs = p_->getVVS().getVariableStorage().getRestrictor(View(v));
        return static_cast< int32_t >(*(my_lower_bound(
            rs.begin(), rs.end(), s, p_->getVVS().getVariableStorage().getOrderStorage(v))));
    }
}


bool PropagatorThread::isModel(Clingo::PropagateControl &control)
{
    auto &vs = p_->getVVS().getVariableStorage();

    Solver s(stats_, trueLit_, control);
    assert(orderLitsAreOK(s));
    //    std::cout << "Is probably a model ?"
    //              << " at dl " << control.assignment().decision_level() << std::endl;


    Variable unrestrictedVariable(InvalidVar);
    uint64_t maxSize = 1;
    for (size_t i = 0; i < vs.numVariables(); ++i)
    {
        uint64_t size = free_range(Variable(i), s);
        if (size > maxSize)
        {
            maxSize = size;
            unrestrictedVariable = Variable(i);
        }
    }

    if (maxSize > 1) /// there is some unknownness
    {
        auto lr = vs.getCurrentRestrictor(View(unrestrictedVariable));
        auto it = lr.begin() + static_cast< int64 >((maxSize - 1) / 2);
        Literal l = s.getNewLiteral();
        p_->getVVS().setLELit(it, l);
        // std::cout << "Added V" << unrestrictedVariable << "<=" << *it << std::endl;
        addWatch(control, unrestrictedVariable, l, it.numElement());
        return false;
    }
    else
    {
        // printPartialState();
        /// store the model to be printed later
        if (names_)
            for (auto it = names_->begin(); it != names_->end(); ++it)
            // for (Variable v = 0; v < p_->getVVS().getVariableStorage().numVariables(); ++v)
            {

                bool fulfilled = false;
                /// or none of the lits is true
                for (auto lit : it->second.second)
                    if (s.isTrue(lit))
                    {
                        fulfilled = true;
                        break;
                    }
                if (!fulfilled)
                {
                    lastModel_.erase(it->first);
                    continue;
                }

                Variable v(it->first);
                assert(p_->getVVS().getVariableStorage().isValid(v));
                lastModel_[v] = value(v, s);
            }
    }

    return true;
}


void PropagatorThread::extend_model(Clingo::Model &m) const
{
    Clingo::SymbolVector vec;
    for (const auto &i : lastModel_)
    {
        Clingo::SymbolVector params;
        params.emplace_back((*names_).at(i.first).first);
        params.emplace_back(Clingo::Number(i.second));
        vec.emplace_back(Clingo::Function("csp", params));
    }
    m.extend(vec);
}

} // namespace clingcon
