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

void ClingconConstraintPropagator::init(Clingo::PropagateInit &init)
{
    ///
    /// if a reification literal is changed, i have to notify p_ and enable/disable a constraint
    /// I just have to shedule this constraint (it is true)
    /// I need to know
    /// 1. is the literal a reification literal             -> (data.bit 1) (sign=false)
    /// 2. the constraint index                             -> data.var
    /// I just need the clasp variable to see which index has to be sheduled for propagation
    ///
    ///

    for (size_t i = 0; i < init.number_of_threads(); ++i)
        constraintProps_.emplace_back(po_.getBase(i));

    /// add watches for undecided reification literals
    auto constraints = constraintProps_[0].base().p_.constraints();
    for (std::size_t cindex = 0; cindex < constraints.size(); ++cindex)
    {
        /// just watch the nonfalse ones
        Literal l = constraints[cindex].v;
        if (!s_.isFalse(l)) /// permanent false otherwise, do not need to consider it
        {
            init.add_watch(l);
            // for (auto const& cpthread : constraintProps_)
            //    cpthread.addWatch(init, l);
            // s.addWatch(-l, this, blob.asUint()); // i'm just watching implications
        }
    }
}

void ClingconOrderPropagator::init(Clingo::PropagateInit &init)
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
    ///
    ///
    ///
    for (size_t i = 0; i < init.number_of_threads(); ++i)
    {
        bases_.emplace_back(s_, vc_, conf_, names_, constraints_, propVar2cspVar_);
    }

    for (const auto &i : bases_) orderProps_.emplace_back(s_, i);

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
}

void ClingconOrderPropagator::propagate(Clingo::PropagateControl &control,
                                        Clingo::LiteralSpan changes)
{
    assert(control.thread_id() < orderProps_.size());
    orderProps_[control.thread_id()].propagate(control, changes);
}

void ClingconConstraintPropagator::propagate(Clingo::PropagateControl &control,
                                             Clingo::LiteralSpan changes)
{
    assert(control.thread_id() < constraintProps_.size());
    constraintProps_[control.thread_id()].propagate(control, changes);
}


void PropagatorThreadBase::propagate(Clingo::PropagateControl &control, Clingo::LiteralSpan changes)
{
    /// only called if p is gets true (otherwise -p gets true)
    for (auto i : changes)
    {
        assert(s_.isTrue(i));
        assert(s_.level(p.var()) == control.assignment().decision_level());
    }

    if (dls_.back() != control.assignment().decision_level())
    {
        // std::cout << "new level " << s.decisionLevel() << std::endl;
        dls_.emplace_back(control.assignment().decision_level());
        p_.addLevel();
        // s_.addUndoWatch(control.assignment().decision_level(), this);
        // std::cout << "Variable storage before getting to the next level " <<
        // p_.getVVS().getVariableStorage() << std::endl;
    }
}


void ClingconOrderPropagatorThread::propagate(Clingo::PropagateControl &control,
                                              Clingo::LiteralSpan changes)
{
    base_.propagate(control, changes);
    s_.beginPropagate(control);

    for (auto p : changes)
    {
        /// order literal
        assert(base_.propVar2cspVar_.find(abs(p)) != base_.propVar2cspVar_.end());
        const auto &cspVars = base_.propVar2cspVar_[abs(p)];

        for (const auto &cspVar : cspVars)
        {
            uint64 bound = abs(cspVar.second) - 1;
            bool sign = cspVar.second < 0;
            //            std::cout << "Propagate order Literal" << std::endl;
            //            std::cout << "Variable: " << cspVar.first << " with bound/sign/p,sign" <<
            //            bound << "/" << sign << "/" << p.sign() << " on dl" << s_.decisionLevel()
            //            << std::endl;
            //            std::cout << "Literal: " << p.rep() << "<-litnumber "<< p.var() << "," <<
            //            p.sign() << " @"<< s_.decisionLevel() << std::endl;

            Restrictor lr = base_.p_.getVVS().getVariableStorage().getRestrictor(
                cspVar.first); //(*orderLits_[cspVar.first]);
            /// these two assertions do not work for incremental
            /// assert(p_.getVVS().getVariableStorage().getDomain(cspVar.first).size() > bound); ///
            /// assure that the given bound is in range
            /// assert(toClaspFormat(p_.getVVS().getVariableStorage().getLELiteral(lr.begin()+bound)).var()
            /// == p.var()); /// given the cspvar+bound we can reinfer the literal which should be p
            /// again

            if ((p < 0 && !sign) || (p > 0 && sign))
            { /// cspVar.first > bound
                if (base_.p_.getVVS()
                        .getVariableStorage()
                        .getCurrentRestrictor(cspVar.first)
                        .begin() < (lr.begin() + bound + 1))
                {
                    bool nonempty = base_.p_.constrainLowerBound(
                        lr.begin() + bound + 1); /// we got not (a<=x) ->the new lower bound is x+1
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
                    (base_.conf_.minLitsPerVar >= 0 &&
                     (uint64)(base_.conf_.minLitsPerVar) <
                         base_.p_.getVVS().getVariableStorage().getDomain(cspVar.first).size()))
                {
                    pure_LELiteral_iterator newit(
                        lr.begin() + bound,
                        base_.p_.getVVS().getVariableStorage().getOrderStorage(cspVar.first),
                        false);
                    while ((--newit).isValid())
                    {
                        if (s_.isFalse(*newit)) break;
                        /// TODO:s_.addClause();
                        if (!s_.addClause(LitVec{*newit, -p})) return false;
                        // if (!s_.force(-(*newit), Clasp::Antecedent(p)))
                        //    return PropResult(false, true);
                    }
                }
            }
            else
            { /// cspVar.first <= bound
                if (base_.p_.getVVS()
                        .getVariableStorage()
                        .getCurrentRestrictor(cspVar.first)
                        .end() > (lr.begin() + bound))
                {
                    bool nonempty = base_.p_.constrainUpperBound(
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

                if (base_.conf_.minLitsPerVar >=
                    0 /*|| !base_.conf_.explicitBinaryOrderClausesIfPossible*/)
                    if (/*!conf_.explicitBinaryOrderClausesIfPossible ||*/
                        (base_.conf_.minLitsPerVar >= 0 &&
                         (uint64)(base_.conf_.minLitsPerVar) <
                             base_.p_.getVVS().getVariableStorage().getDomain(cspVar.first).size()))
                    {
                        pure_LELiteral_iterator newit(
                            lr.begin() + bound,
                            base_.p_.getVVS().getVariableStorage().getOrderStorage(cspVar.first),
                            true);
                        while ((++newit).isValid())
                        {
                            if (s_.isTrue(*newit)) break;
                            if (!s_.addClause(LitVec{-(*newit), -p})) return false;
                            // if (!s_.force(toClaspFormat(*newit), Clasp::Antecedent(p)))
                            //    return PropResult(false, true);
                        }
                    }
            }
        }
    }
    control.propagate();
}

void ClingconConstraintPropagatorThread::propagate(Clingo::PropagateControl &control,
                                                   Clingo::LiteralSpan changes)
{
    base_.propagate(control, changes);
    s_.beginPropagate(control);

    for (auto i : changes) base_.p_.queueConstraint(var2Constraints_[abs(i)]);

    assert(!assertConflict_);
    assert(orderLitsAreOK());
    while (!base_.p_.atFixPoint())
    {
        const auto &clauses = base_.p_.propagateSingleStep();
        auto &vs = base_.p_.getVVS();
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
                            addWatch(varit.view().v, -toClaspFormat(l), varit.numElement() - 1);
                        }
                        else
                        {
                            auto varit = ViewIterator::viewToVarIterator(it);
                            addWatch(varit.view().v, toClaspFormat(l), varit.numElement());
                        }
                    }

                    /// now it has a literal
                    {
                        auto currentIt =
                            base_.p_.getVVS().getVariableStorage().getCurrentRestrictor(
                                its[i].view());
                        Literal l = vs.getVariableStorage().getGELiteral(its[i]);
                        if (s_.isUnknown(l))
                        {
                            auto it = its[i] - 1;
                            if (it < currentIt.begin())
                            {
                                if (it.view().reversed())
                                {
                                    forceKnownLiteralLE(it, l);
                                }
                                else
                                {
                                    forceKnownLiteralGE(it, l);
                                }
                            }
                            else if (currentIt.end() <= it)
                            {
                                if (it.view().reversed())
                                {
                                    forceKnownLiteralGE(it, l);
                                }
                                else
                                {
                                    forceKnownLiteralLE(it, l);
                                }
                            }
                            /// else it simply free and unit asserting
                        }

                        claspClause.push_back(-l);
                    }
                }

                ////1=true, 2=false, 0=free
                /// all should be isFalse(i)==true, one can be unknown or false
                //                              std::cout << "Give clasp the clause " << std::endl;
                //                              for (auto i : claspClause)
                //                                  std::cout << i.rep() << "@" << s_.level(i.var())
                //                                  << "isFalse?" << s_.isFalse(i) << " isTrue" <<
                //                                  s_.isTrue(i) << "  ,   ";
                //                              std::cout << " on level " << s_.decisionLevel();
                //                              std::cout << std::endl;
                assert(std::count_if(claspClause.begin(), claspClause.end(), [&](Clasp::Literal i) {
                           return s_.isFalse(i);
                       }) >= claspClause.size() - 1);
                assert(std::count_if(claspClause.begin(), claspClause.end(), [&](Clasp::Literal i) {
                           return s_.isFalse(i) && (s_.level(i.var()) == s_.decisionLevel());
                       }) >= 1);


                if (!s_.addClause(claspClause)) return;
            }
            if (!control.propagate())
            {
                return false;
            }
            assert(orderLitsAreOK());
        }
    }
    return true;
}


void ClingconOrderPropagator::check(Clingo::PropagateControl &control) {}
void ClingconOrderPropagatorThread::check(Clingo::PropagateControl &control) {}
void ClingconConstraintPropagator::check(Clingo::PropagateControl &control) {}
void ClingconConstraintPropagatorThread::check(Clingo::PropagateControl &control) {}

void PropagatorThreadBase::undo(Clingo::PropagateControl const &control)
{
    // std::cout << "reset on dl " << s_.decisionLevel() << std::endl;
    //        if (control.assignment().decision_level() != 0 &&
    //        control.assignment().decision_level() == base_.dls_.back())
    //        {
    //            base_.p_.removeLevel();
    //            base_.p_.addLevel();
    //        }
    //        else
    while (control.assignment().decision_level() <= base_.dls_.back())
    {
        p_.removeLevel();
        dls_.pop_back();
    }
}


void ClingconOrderPropagator::undo(Clingo::PropagateControl const &control)
{
    assert(control.thread_id() < orderProps_.size());
    orderProps_[control.thread_id()].undo(control, changes);
}
void ClingconOrderPropagatorThread::undo(Clingo::PropagateControl const &control,
                                         Clingo::LiteralSpan changes)
{
    assertConflict_ = false;
    base_.undo(control);
}
void ClingconConstraintPropagator::undo(Clingo::PropagateControl const &control,
                                        Clingo::LiteralSpan changes)
{
    assert(control.thread_id() < constraintProps_.size());
    constraintProps_[control.thread_id()].undo(control, changes);
}
void ClingconConstraintPropagatorThread::undo(Clingo::PropagateControl &control,
                                              LiteralSpan changes)
{
    base_.undo(control);
}


void ClingconOrderPropagator::reason(Clasp::Solver &, Clasp::Literal p, Clasp::LitVec &lits)
{
    if (conflict_.size())
    {
        lits.insert(lits.end(), conflict_.begin(), conflict_.end());
        conflict_.clear();
    }
    else
        lits.insert(lits.end(), reasons_[p.var()].begin(), reasons_[p.var()].end());
}


void ClingconOrderPropagator::forceKnownLiteralGE(ViewIterator it, Clasp::Literal l)
{
    // assert(iterator is variterator)
    auto varit = ViewIterator::viewToVarIterator(it);
    pure_LELiteral_iterator pit(
        varit, p_.getVVS().getVariableStorage().getOrderStorage(varit.view().v), 1);
    assert(pit.isValid());
    ++pit;
    assert(pit.isValid());
    while (!s_.isFalse(toClaspFormat(*pit)))
    {
        assert(!s_.isTrue(toClaspFormat(*pit)));
        ++pit;
        assert(pit.isValid());
    }
    s_.force(l, Clasp::Antecedent(-(toClaspFormat(*pit))));
}

void ClingconOrderPropagator::forceKnownLiteralLE(ViewIterator it, Clasp::Literal l)
{
    auto varit = ViewIterator::viewToVarIterator(it);
    pure_LELiteral_iterator pit(
        varit, p_.getVVS().getVariableStorage().getOrderStorage(varit.view().v), 0);
    assert(pit.isValid());
    --pit;
    assert(pit.isValid());
    while (!s_.isTrue(toClaspFormat(*pit)))
    {
        assert(!s_.isFalse(toClaspFormat(*pit)));
        --pit;
        assert(pit.isValid());
    }
    s_.force(l, Clasp::Antecedent(toClaspFormat(*pit)));
}


bool ClingconOrderPropagator::propagateFixpoint(Clasp::Solver &, PostPropagator *) {}


void ClingconOrderPropagator::addWatch(Clingo::PropagateInit &init, const Variable &var, Literal cl,
                                       unsigned int step)
{
    init.add_watch(cl);
    init.add_watch(-cl);
    int32 x = (cl < 0) ? int32(step + 1) * -1 : int32(step + 1);
    for (auto &i : bases_) i.propVar2cspVar_[cl.var()].emplace_back(std::make_pair(var, x));
}

void ClingconConstraintPropagatorThread::addWatch(Clingo::PropagateControl &control,
                                                  const Variable &var, Literal cl,
                                                  unsigned int step)
{
    DataBlob blob(0, true);
    control.add_watch(cl);
    control.add_watch(-cl);
    int32 x = cl.sign() ? int32(step + 1) * -1 : int32(step + 1);
    base_.propVar2cspVar_[cl.var()].emplace_back(std::make_pair(var, x));
}


/// debug function to check if stored domain restrictions are in line with order literal assignment
/// in clasp solver
/// furthermore check if current bound has literals (upper and lower must have lits)
/// THIS CONDITION IS NO LONGER TRUE,
/// I CAN HAVE UNDECIDED VARIABLES OUTSIDE OT THE BOUNDS
/// THEY WILL BE SET IF NEEDED
bool ClingconConstraintPropagatorThread::orderLitsAreOK() { /*
     for (Variable var = 0; var < p_.getVVS().getVariableStorage().numVariables(); ++var)
     {
         if (watched_[var] && p_.getVVS().getVariableStorage().isValid(var))
         {
             //std::cout << "check var " << var << std::endl;
             auto currentlr = p_.getVVS().getVariableStorage().getCurrentRestrictor(var);
             auto baselr = p_.getVVS().getVariableStorage().getRestrictor(var);

             //std::cout << "Orig domain " << baselr.lower() << ".." << baselr.upper() << " is "
             //          << currentlr.lower() << ".." << currentlr.upper() << std::endl;

             pure_LELiteral_iterator
 it(baselr.begin(),p_.getVVS().getVariableStorage().getOrderStorage(var),true);

             if (!it.isValid())
                 assert(false);
             while(it.isValid())
             {
                 //std::cout << "checking element " << it.numElement() << std::endl;
                 if (s_.isFalse(toClaspFormat(*it)) && it.numElement() >=
 currentlr.begin().numElement())
                     assert(false);
                 if (s_.isTrue(toClaspFormat(*it)) && it.numElement() <
 currentlr.end().numElement()-1)
                     assert(false);
                 if (!s_.isFalse(toClaspFormat(*it)) && !s_.isTrue(toClaspFormat(*it)) &&
 (it.numElement() < currentlr.begin().numElement() || it.numElement() >=
 currentlr.end().numElement()))
                 {
 //                    std::cout << "checking literal " << (*it).asUint() << " mit elementindex " <<
 it.numElement() << std::endl;
 //                    std::cout << "this is v" << var << "<=" << *(baselr.begin()+it.numElement())
 << std::endl;
 //                    std::cout << "Orig domain " << baselr.lower() << ".." << baselr.upper() << "
 is "
 //                              << currentlr.lower() << ".." << currentlr.upper() << std::endl;
                     assert(false);
                 }
                 ++it;
             }
         }
     }*/ return true; }


void ClingconOrderPropagator::reset()
{
    // std::cout << "reset on dl " << s_.decisionLevel() << std::endl;
    assertConflict_ = false;
    if (s_.decisionLevel() != 0 && s_.decisionLevel() == dls_.back())
    {
        p_.removeLevel();
        p_.addLevel();
    }
}


void ClingconOrderPropagator::undoLevel(Clasp::Solver &)
{
    // std::cout << "undo dl " << s_.decisionLevel() << std::endl;
    assertConflict_ = false;
    p_.removeLevel();
    dls_.pop_back();
}


namespace
{

    /// finds first literal that is true

    template < class ForwardIt >
    ForwardIt my_upper_bound(ForwardIt first, ForwardIt last, Clasp::Solver &s,
                             const orderStorage &os)
    {
        ForwardIt it;
        typename std::iterator_traits< ForwardIt >::difference_type count, step;
        count = std::distance(first, last);

        while (count > 0)
        {
            it = first;
            step = count / 2;
            std::advance(it, step);
            if (s.isFalse(toClaspFormat(os.getLiteral(it.numElement()))))
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

bool ClingconOrderPropagator::isModel(Clasp::Solver &)
{
    // std::cout << "Is probably a model ?" << " at dl " << s_.decisiconstraints_onLevel() <<
    // std::endl;
    auto &vs = p_.getVVS().getVariableStorage();

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
        addWatch(unrestrictedVariable, toClaspFormat(l), it.numElement());
        return false;
    }
    else
    {
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
                EqualityClass::Edge e(1, 1, 0);
                // const EqualityProcessor::EqualityClassMap::iterator eqsit;
                Variable var(v);
                if (!p_.getVVS().getVariableStorage().isValid(v))
                {
                    auto eqsit = eqs_.find(v);
                    assert(eqsit != eqs_.end()); // should be inside, otherwise variable is valid
                    assert(eqsit->second->top() !=
                           v); // should not be top variable -> otherwise variable is valid
                    auto &constraints = eqsit->second->getConstraints();
                    assert(constraints.find(v) != constraints.end());
                    e = constraints.find(v)->second;
                    var = eqsit->second->top();
                }

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
                int32 value = (((int64)(e.secondCoef) * (int64)(*vit)) + (int64)(e.constant)) /
                              (int64)(e.firstCoef);
                assert((((int64)(e.secondCoef) * (int64)(*vit)) + (int64)(e.constant)) %
                           (int64)(e.firstCoef) ==
                       0);
                lastModel_[v] = value;
            }
    }

    return true;
}


const char *ClingconOrderPropagator::printModel(Variable v, const std::string &name)
{
    auto find = lastModel_.find(v);
    if (find == lastModel_.end()) return 0;
    // std::cout << "enter printModel " << v << " " << name << std::endl;
    outputbuf_ = name;
    outputbuf_ += "=";
    outputbuf_ += std::to_string(find->second);
    return outputbuf_.c_str();
}

bool ClingconOrderPropagator::getValue(Variable v, int32 &value)
{
    auto find = lastModel_.find(v);
    if (find == lastModel_.end()) return false;
    value = find->second;
    return true;
}
}
