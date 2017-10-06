// {{{ GPL License

// This file is part of libcsp - a library for handling linear constraints.
// Copyright (C) 2016  Max Ostrowski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

    /// if an orderLiteral changes, i change lower/upper bound p_.changeUpperBound(variable,
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
    init.set_check_mode(Clingo::PropagatorCheckMode::Partial);

    /// convert aspif literals to solver literals
    trueLit_ = init.solver_literal(trueLit_);
    s_.init(trueLit_);
    vc_.convertLiterals(init);
    for (auto &i : constraints_) i.v = init.solver_literal(i.v);
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
        if (vc_.isValid(var))
        {
            // if (!conf_.explicitBinaryOrderClausesIfPossible)
            watched_[var] = true;
            // slse if (vc_.numOrderLits(var) < vc_.getDomain(var).size())
            //    watched_[var] = true;
        }
    }

    for (std::size_t var = 0; var != watched_.size(); ++var)
    {
        if (watched_[var])
        {
            auto lr = pure_LELiteral_iterator(vc_.getRestrictor(View(var)).begin(),
                                              vc_.getStorage(var), true);

            while (lr.isValid())
            {
                /// TODO: do i need to do this for true lits ?
                addWatch(init, (Variable)(var), *lr, lr.numElement());
                ++lr;
            }
        }
    }

    for (size_t i = 0; i < init.number_of_threads(); ++i)
        threads_.emplace_back(s_, vc_, conf_, names_, constraints_, propVar2cspVar_, watched_);

    propVar2cspVar_.clear();
    constraints_.clear();
}


void ClingconPropagator::propagate(Clingo::PropagateControl &control, Clingo::LiteralSpan changes)
{
    assert(control.thread_id() < threads_.size());
    threads_[control.thread_id()].propagate(control, changes);
}

void ClingconPropagator::check(Clingo::PropagateControl &control)
{
    assert(control.thread_id() < threads_.size());
    threads_[control.thread_id()].check(control);
}


void PropagatorThread::propagate(Clingo::PropagateControl &control, Clingo::LiteralSpan changes)
{
    s_.beginPropagate(control);
    /// only called if p is gets true (otherwise -p gets true)
    for (auto i : changes)
    {
        assert(p_.getSolver().isTrue(i));
        assert(control.assignment().level(i) == control.assignment().decision_level());
    }

    if (dls_.back() != control.assignment().decision_level())
    {
        // std::cout << "new level " << control.assignment().decision_level() << std::endl;
        dls_.emplace_back(control.assignment().decision_level());
        assert(p_.atFixPoint());
        p_.addLevel();
        // s_.addUndoWatch(control.assignment().decision_level(), this);
        // std::cout << "Variable storage before getting to the next level " <<
        // p_.getVVS().getVariableStorage() << std::endl;
    }

    propagateOrderVariables(control, changes);
}

void PropagatorThread::check(Clingo::PropagateControl &control)
{
    s_.beginPropagate(control);
    assert(orderLitsAreOK());
    printPartialState();
    if (!propagateConstraintVariables(control)) return;
    printPartialState();
    if (control.assignment().is_total()) isModel(control);
}


void PropagatorThread::printPartialState()
{
    auto& vs = p_.getVVS().getVariableStorage();
    for (Variable i = 0; i < vs.numVariables(); ++i)
    {
        auto r = vs.getCurrentRestrictor(i);
        std::cout << "var" << i << "\t= " << r.lower() << "\t.. " << r.upper() << std::endl;
        if (!vs.hasLELiteral(r.end()-1))
            std::cout << "upper bound has no LE literal" << std::endl;
        else
            if (!s_.isTrue(vs.getLELiteral(r.end()-1)))
                std::cout << "upper bound literal is not true" << std::endl;
        if (!vs.hasGELiteral(r.begin()))
            std::cout << "lower bound has no GE literal" << std::endl;
        else
            if (!s_.isTrue(vs.getGELiteral(r.begin())))
                std::cout << "lower bound GE literal is not true" << std::endl;
    }

    auto it = vs.getRestrictor(0).begin()+1073741827;
    if (vs.hasGELiteral(it))
        std::cout << "value of v0>="<< *it << " with literal " << vs.getGELiteral(it)  << " being " << s_.isTrue(vs.getGELiteral(it)) << "/" << s_.isFalse(vs.getGELiteral(it)) << std::endl;
}


void PropagatorThread::propagateOrderVariables(Clingo::PropagateControl &control,
                                               Clingo::LiteralSpan changes)
{

    for (auto p : changes)
    {
        auto constraints = var2Constraints_.find(abs(p));
        if (constraints != var2Constraints_.end())
            for (auto c : constraints->second) p_.queueConstraint(c);
        /// order literal
        auto found = propVar2cspVar_.find(abs(p));
        if (found == propVar2cspVar_.end()) continue;
        const auto &cspVars = found->second;

        for (const auto &cspVar : cspVars)
        {
            // printPartialState();
            uint64 bound = abs(cspVar.second) - 1;
            bool sign = cspVar.second < 0;
            //            std::cout << "Propagate order Literal" << std::endl;
            //            std::cout << "Variable: " << cspVar.first << " with bound/sign/p,sign" <<
            //            bound << "/"
            //                      << sign << "/" << ((p < 0) ? -1 : 1) << " on dl"
            //                      << control.assignment().decision_level() << std::endl;
            //            std::cout << "Literal: " << p << " @" <<
            //            control.assignment().decision_level()
            //                      << std::endl;

            Restrictor lr = p_.getVVS().getVariableStorage().getRestrictor(
                cspVar.first); //(*orderLits_[cspVar.first]);
            /// these two assertions do not work for incremental
            /// assert(p_.getVVS().getVariableStorage().getDomain(cspVar.first).size() > bound); ///
            /// assure that the given bound is in range
            /// assert(toClaspFormat(p_.getVVS().getVariableStorage().getLELiteral(lr.begin()+bound)).var()
            /// == p.var()); /// given the cspvar+bound we can reinfer the literal which should be p
            /// again

            if ((p < 0 && !sign) || (p > 0 && sign))
            { /// cspVar.first > bound
                // std::cout << "new lower bound " << *(lr.begin() + bound + 1) << std::endl;
                if (p_.getVVS().getVariableStorage().getCurrentRestrictor(cspVar.first).begin() <
                    (lr.begin() + bound + 1))
                {
                    bool nonempty = p_.constrainLowerBound(
                        lr.begin() + bound + 1); /// we got not (a<=x) ->the new lower bound is x+1
                    // printPartialState();
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
                if (/*!base_.conf_.explicitBinaryOrderClausesIfPossible ||*/
                    (conf_.minLitsPerVar >= 0 &&
                     (uint64)(conf_.minLitsPerVar) <
                         p_.getVVS().getVariableStorage().getDomain(cspVar.first).size()))
                {
                    pure_LELiteral_iterator newit(
                        lr.begin() + bound,
                        p_.getVVS().getVariableStorage().getOrderStorage(cspVar.first), false);
                    while ((--newit).isValid())
                    {
                        if (s_.isFalse(*newit)) break;
                        /// TODO:s_.addClause();
                        if (!s_.addClause(LitVec{-(*newit), -p})) return;
                        // if (!s_.force(-(*newit), Clasp::Antecedent(p)))
                        //    return PropResult(false, true);
                    }
                }
            }
            else
            { /// cspVar.first <= bound
                // std::cout << "new upper bound " << *(lr.begin() + bound) << std::endl;
                if (p_.getVVS().getVariableStorage().getCurrentRestrictor(cspVar.first).end() >
                    (lr.begin() + bound))
                {
                    bool nonempty = p_.constrainUpperBound(
                        lr.begin() + bound + 1); /// we got a <= x, the new end restrictor is at x+1
                    // assert(nonempty);
                    if (!nonempty)
                    { /*std::cout << "now" << std::endl;*/
                        assertConflict_ = true;
                    }
                }
                /// the end of the free range was (probably) shifted to the left
                /// if range was restricted from 5...100 to 5..95, we make 96,97,98,99 true with
                /// reason x<95

                if (conf_.minLitsPerVar >=
                    0 /*|| !base_.conf_.explicitBinaryOrderClausesIfPossible*/)
                    if (/*!conf_.explicitBinaryOrderClausesIfPossible ||*/
                        (conf_.minLitsPerVar >= 0 &&
                         (uint64)(conf_.minLitsPerVar) <
                             p_.getVVS().getVariableStorage().getDomain(cspVar.first).size()))
                    {
                        pure_LELiteral_iterator newit(
                            lr.begin() + bound,
                            p_.getVVS().getVariableStorage().getOrderStorage(cspVar.first), true);
                        while ((++newit).isValid())
                        {
                            if (s_.isTrue(*newit)) break;
                            if (!s_.addClause(LitVec{*newit, -p})) return;
                            // if (!s_.force(toClaspFormat(*newit), Clasp::Antecedent(p)))
                            //    return PropResult(false, true);
                        }
                    }
            }
        }
    }
    control.propagate();
}

bool PropagatorThread::propagateConstraintVariables(Clingo::PropagateControl &control)
{
     std::cout << "propagateConstraintVariables" << std::endl;
    assert(!assertConflict_);
    assert(orderLitsAreOK());
    printPartialState();
    while (!p_.atFixPoint())
    {
        const auto &clauses = p_.propagateSingleStep();
        printPartialState();
        auto &vs = p_.getVVS();
        if (clauses.size())
        {
            for (const auto &clause : clauses)
            {
                LitVec claspClause;
                claspClause.push_back(clause.first);

                const auto &its = clause.second;

                for (unsigned int i = 0; i < its.size(); ++i)
                {
                    if (!vs.getVariableStorage().hasGELiteral(its[i]))
                    {
                        Literal l = s_.getNewLiteral();
                        auto it = its[i] - 1;
                        vs.setLELit(it, l);
                        // claspClause.push_back(toClaspFormat(l));

                        if (it.view().reversed())
                        {
                            auto varit = ViewIterator::viewToVarIterator(it);
                            addWatch(control, varit.view().v, -l, varit.numElement() - 1);
                        }
                        else
                        {
                            auto varit = ViewIterator::viewToVarIterator(it);
                            addWatch(control, varit.view().v, l, varit.numElement());
                        }
                    }

                    /// now it has a literal
                    {
                        auto currentIt =
                            p_.getVVS().getVariableStorage().getCurrentRestrictor(its[i].view());
                        Literal l = vs.getVariableStorage().getGELiteral(its[i]);
                        if (s_.isUnknown(l))
                        {
                            auto it = its[i] - 1;
                            if (it < currentIt.begin())
                            {
                                if (it.view().reversed())
                                {
                                    if (forceKnownLiteralLE(it, l)) return false;
                                }
                                else
                                {
                                    if (!forceKnownLiteralGE(it, l)) return false;
                                }
                            }
                            else if (currentIt.end() <= it)
                            {
                                if (it.view().reversed())
                                {
                                    if (!forceKnownLiteralGE(it, l)) return false;
                                }
                                else
                                {
                                    if (!forceKnownLiteralLE(it, l)) return false;
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
                //                    std::cout << i << "@" << control.assignment().level(i) <<
                //                    "isFalse?"
                //                              << s_.isFalse(i) << " isTrue" << s_.isTrue(i) << "
                //                              ,   ";
                //                std::cout << " on level " <<
                //                control.assignment().decision_level();
                //                std::cout << std::endl;
                assert(std::count_if(claspClause.begin(), claspClause.end(), [&](Literal i) {
                           return s_.isFalse(i);
                       }) >= claspClause.size() - 1);
                assert(std::count_if(claspClause.begin(), claspClause.end(), [&](Literal i) {
                           return s_.isFalse(i) && (control.assignment().level(i) ==
                                                    control.assignment().decision_level());
                       }) >= 1);


                printPartialState();
                if (!s_.addClause(claspClause)) return false;
                printPartialState();
            }
        }
        size_t oldsize = control.assignment().size();
        if (!control.propagate()) return false;
        printPartialState();
        if (oldsize < control.assignment().size())
            return true; /// do other propagation first
    }
    return control.propagate();
}


void ClingconPropagator::undo(Clingo::PropagateControl const &control, Clingo::LiteralSpan changes)
{
    assert(control.thread_id() < threads_.size());
    threads_[control.thread_id()].undo(control, changes);
}

void PropagatorThread::undo(Clingo::PropagateControl const &control, Clingo::LiteralSpan)
{
    // std::cout << "reset on dl " << control.assignment().decision_level() << std::endl;
    //        if (control.assignment().decision_level() != 0 &&
    //        control.assignment().decision_level() == base_.dls_.back())
    //        {
    //            base_.p_.removeLevel();
    //            base_.p_.addLevel();
    //        }
    //        else
    assertConflict_ = false;
    while (control.assignment().decision_level() <= dls_.back() && dls_.size() > 1)
    {
        p_.removeLevel();
        dls_.pop_back();
    }
}


bool PropagatorThread::forceKnownLiteralGE(ViewIterator it, Literal l)
{
    // assert(iterator is variterator)
    auto varit = ViewIterator::viewToVarIterator(it);
    pure_LELiteral_iterator pit(
        varit, p_.getVVS().getVariableStorage().getOrderStorage(varit.view().v), 1);
    assert(pit.isValid());
    ++pit;
    assert(pit.isValid());
    while (!s_.isFalse(*pit))
    {
        assert(!s_.isTrue(*pit));
        ++pit;
        assert(pit.isValid());
    }
    // s_.force(l, Clasp::Antecedent(-(toClaspFormat(*pit))));
    // std::cout << s_.isTrue(l) << " " << s_.isFalse(l) << std::endl;
    return s_.addClause({l, *pit}); /// can fail if *pit was assigned on a former level
}

bool PropagatorThread::forceKnownLiteralLE(ViewIterator it, Literal l)
{
    auto varit = ViewIterator::viewToVarIterator(it);
    pure_LELiteral_iterator pit(
        varit, p_.getVVS().getVariableStorage().getOrderStorage(varit.view().v), 0);
    assert(pit.isValid());
    --pit;
    assert(pit.isValid());
    while (!s_.isTrue(*pit))
    {
        assert(!s_.isFalse(*pit));
        --pit;
        assert(pit.isValid());
    }
    // s_.force(l, Clasp::Antecedent(toClaspFormat(*pit)));
    return s_.addClause({l, -(*pit)}); /// can fail if *pit was assigned on a former level
}


void ClingconPropagator::addWatch(Clingo::PropagateInit &init, const Variable &var, Literal cl,
                                  unsigned int step)
{
    // std::cout << "OrderProp watch " << cl << " and " << -cl << std::endl;
    init.add_watch(cl);
    init.add_watch(-cl);
    int32 x = (cl < 0) ? int32(step + 1) * -1 : int32(step + 1);
    propVar2cspVar_[abs(cl)].emplace_back(std::make_pair(var, x));
}

void PropagatorThread::addWatch(Clingo::PropagateControl &control, const Variable &var, Literal cl,
                                unsigned int step)
{
    if (cl==5053)
        int a = 7;
    control.add_watch(cl);
    control.add_watch(-cl);
    int32 x = cl < 0 ? int32(step + 1) * -1 : int32(step + 1);
    propVar2cspVar_[abs(cl)].emplace_back(std::make_pair(var, x));
}


/// debug function to check if stored domain restrictions are in line with order literal assignment
/// in clasp solver
/// furthermore check if current bound has literals (upper and lower must have lits)
/// THIS CONDITION IS NO LONGER TRUE,
/// I CAN HAVE UNDECIDED VARIABLES OUTSIDE OT THE BOUNDS
/// THEY WILL BE SET IF NEEDED --- really? how
bool PropagatorThread::orderLitsAreOK() {
     for (Variable var = 0; var < p_.getVVS().getVariableStorage().numVariables(); ++var)
     {
         if (watched_[var] && p_.getVVS().getVariableStorage().isValid(var))
         {
             std::cout << "check var " << var << std::endl;
             auto currentlr = p_.getVVS().getVariableStorage().getCurrentRestrictor(var);
             auto baselr = p_.getVVS().getVariableStorage().getRestrictor(var);

             std::cout << "Orig domain " << baselr.lower() << ".." << baselr.upper() << " is "
                       << currentlr.lower() << ".." << currentlr.upper() << std::endl;

             pure_LELiteral_iterator
 it(baselr.begin(),p_.getVVS().getVariableStorage().getOrderStorage(var),true);

             if (!it.isValid())
                 assert(false);
             while(it.isValid())
             {
                 //std::cout << "checking element " << it.numElement() << std::endl;
                 if (s_.isFalse(*it) && it.numElement() >=
 currentlr.begin().numElement())
                     assert(false);
                 if (s_.isTrue(*it) && it.numElement() <
 currentlr.end().numElement()-1)
                     assert(false);
                 if (!s_.isFalse(*it) && !s_.isTrue(*it) &&
 (it.numElement() < currentlr.begin().numElement() || it.numElement() >=
 currentlr.end().numElement()))
                 {
                     std::cout << "checking literal " << (*it) << " mit elementindex " <<
                     it.numElement() << std::endl;
                     std::cout << "this is v" << var << "<=" << *(baselr.begin()+it.numElement())
                     << std::endl;
                     std::cout << "Orig domain " << baselr.lower() << ".." << baselr.upper() <<
                                  "is " << currentlr.lower() << ".." << currentlr.upper() << std::endl;
                     assert(false);
                 }
                 ++it;
             }
         }
     } return true; }


namespace
{

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
}


bool PropagatorThread::isModel(Clingo::PropagateControl &control)
{
    auto &vs = p_.getVVS().getVariableStorage();

    printPartialState();
    assert(orderLitsAreOK());
    //    std::cout << "Is probably a model ?"
    //              << " at dl " << control.assignment().decision_level() << std::endl;


    Variable unrestrictedVariable(InvalidVar);
    unsigned int maxSize = 1;
    for (unsigned int i = 0; i < vs.numVariables(); ++i)
    {
        if (p_.getVVS().getVariableStorage().isValid(i) && watched_[i])
        {
            auto lr = vs.getCurrentRestrictor(i);

            if (lr.size() > maxSize)
            {
                maxSize = lr.size();
                unrestrictedVariable = i;
            }
        }
    }

    if (maxSize > 1) /// there is some unknownness
    {
        auto lr = vs.getCurrentRestrictor(View(unrestrictedVariable));
        auto it = lr.begin() + ((maxSize - 1) / 2);
        Literal l = p_.getSolver().getNewLiteral();
        p_.getVVS().setLELit(it, l);
        // std::cout << "Added V" << unrestrictedVariable << "<=" << *it << std::endl;
        addWatch(control, unrestrictedVariable, l, it.numElement());
        return false;
    }
    else
    {
        static int c=1;
        std::cout << "FOUND MODEL " << c << std::endl; // this can actually be called twice per model
        // if check is recalled with total assignment
        if (c==19)
            int a = 0;
        ++c;
        /// store the model to be printed later
        if (names_)
            for (auto it = names_->begin(); it != names_->end(); ++it)
            // for (Variable v = 0; v < p_.getVVS().getVariableStorage().numVariables(); ++v)
            {

                bool fulfilled = false;
                /// or none of the lits is true
                for (auto lit : it->second.second)
                    if (s_.isTrue(lit))
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

                Variable var(v);
                assert(p_.getVVS().getVariableStorage().isValid(v));


                ViewIterator vit;
                if (watched_[var])
                {
                    Restrictor lr;
                    lr = p_.getVVS().getVariableStorage().getCurrentRestrictor(var);
                    assert(lr.size() == 1);
                    vit = lr.begin();
                }
                else /// not watched, need to search for value
                {
                    auto rs = p_.getVVS().getVariableStorage().getRestrictor(View(var));
                    vit = my_upper_bound(rs.begin(), rs.end(), s_,
                                         p_.getVVS().getVariableStorage().getOrderStorage(var));
                }
                int32 value = *vit;
                lastModel_[v] = value;
            }
    }

    return true;
}


void PropagatorThread::printAssignment() const
{
    std::cout << "with assignment:\n";
    for (const auto &i : lastModel_)
        std::cout << (*names_).at(i.first).first << ":" << i.second << " ";
    std::cout << "\n";
}
}
