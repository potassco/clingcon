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

#include <clingcon/normalizer.h>
#include <clingcon/translator.h>
#include <clingcon/types.h>

#include <map>
#include <memory>
#include <unordered_map>


namespace clingcon
{


void Normalizer::addConstraint(ReifiedLinearConstraint &&l)
{
    linearConstraints_.emplace_back(std::move(l));
}

void Normalizer::addConstraint(ReifiedDomainConstraint &&d)
{
    domainConstraints_.emplace_back(std::move(d));
}


/// hallsize=0 should be equal to quadratic amount of unequal ?
void Normalizer::addConstraint(ReifiedAllDistinct &&l) { allDistincts_.emplace_back(std::move(l)); }


void Normalizer::addMinimize(View &v, unsigned int level) { minimize_.emplace_back(v, level); }


bool Normalizer::convertLinear(ReifiedLinearConstraint &&l,
                               std::vector< ReifiedLinearConstraint > &insert)
{
    Direction impl = l.impl;
    if ((impl == Direction::FWD && s_.isFalse(l.v)) || (impl == Direction::BACK && s_.isTrue(l.v)))
        return true;
    l.normalize();
    l.impl = Direction::FWD;
    assert(l.l.getRelation() == LinearConstraint::Relation::LE ||
           l.l.getRelation() == LinearConstraint::Relation::EQ ||
           l.l.getRelation() == LinearConstraint::Relation::NE);

    if (l.l.getRelation() == LinearConstraint::Relation::LE)
    {
        if (l.l.getConstViews().size() == 1 && impl == Direction::EQ) // In this case we sometimes
                                                                      // can make an orderLiteral
                                                                      // out of it, if it was not
                                                                      // yet otheriwse created
        {
            // ReifiedLinearConstraint copy(l);
            View v = *l.l.getConstViews().begin();
            int rhs = l.l.getRhs();
            // rhs = v.divide(rhs);
            Restrictor r = vc_.getRestrictor(v);
            auto it = wrap_upper_bound(r.begin(), r.end(), rhs);
            if (it != r.begin())
                --it;
            else
                it = r.end();
            return vc_.setLELit(it, l.v);
        }
        else
        {
            ReifiedLinearConstraint t(l);
            if (!s_.isFalse(l.v) && (impl & Direction::FWD)) insert.emplace_back(std::move(l));
            if (!s_.isTrue(t.v) && (impl & Direction::BACK))
            {
                t.reverse();
                t.v = -t.v;
                insert.emplace_back(std::move(t));
            }
        }
    }
    else if (l.l.getRelation() == LinearConstraint::Relation::EQ)
    {
        assert(l.l.getRelation() == LinearConstraint::Relation::EQ);
        if (l.l.getConstViews().size() == 1 && impl == Direction::EQ) // In this case we sometimes
                                                                      // can make an orderLiteral
                                                                      // out of it, if it was not
                                                                      // yet otheriwse created
        {
            View v = *(l.l.getConstViews().begin());
            Restrictor r = vc_.getRestrictor(v);
            auto it = wrap_lower_bound(r.begin(), r.end(), l.l.getRhs());
            it = (it == r.end() || *it != l.l.getRhs()) ? r.end() : it;
            return vc_.setEqualLit(it, l.v); // otherwise, an equality will be created
        }
        Literal orig = l.v;
        ReifiedLinearConstraint u(l);
        ReifiedLinearConstraint less(l);
        ReifiedLinearConstraint more(l);
        if (!s_.isFalse(l.v) && (impl & Direction::FWD))
        {
            l.l.setRelation(LinearConstraint::Relation::LE);
            insert.emplace_back(std::move(l));
            u.l.setRelation(LinearConstraint::Relation::GE);
            insert.emplace_back(std::move(u));
        }
        if (!s_.isTrue(orig) && (impl & Direction::BACK))
        {
            Literal x = s_.createNewLiteral();
            less.v = x;
            less.l.setRelation(LinearConstraint::Relation::LT);
            insert.emplace_back(std::move(less));
            Literal y = s_.createNewLiteral();
            more.v = y;
            more.l.setRelation(LinearConstraint::Relation::GT);
            insert.emplace_back(std::move(more));
            if (!s_.createClause(
                    LitVec{-x, -orig})) /// having orig implies not x (having x implies not orig)
                return false;
            if (!s_.createClause(
                    LitVec{-y, -orig})) /// having orig implies not y (having y implies not orig)
                return false;
            if (!s_.createClause(LitVec{-x, -y})) /// cant be less and more at same time
                return false;
            if (!s_.createClause(LitVec{x, y, orig})) /// either equal or less or more
                return false;
        }
    }
    else if (l.l.getRelation() == LinearConstraint::Relation::NE)
    {
        if (l.l.getConstViews().size() == 1 && impl == Direction::EQ) // In this case we sometimes
                                                                      // can make an orderLiteral
                                                                      // out of it, if it was not
                                                                      // yet otheriwse created
        {
            View v = *(l.l.getConstViews().begin());
            Restrictor r = vc_.getRestrictor(v);
            auto it = wrap_lower_bound(r.begin(), r.end(), l.l.getRhs());
            it = (it == r.end() || *it != l.l.getRhs()) ? r.end() : it;
            return vc_.setEqualLit(it, -l.v);
        }
        Literal orig = l.v;
        ReifiedLinearConstraint u(l);
        ReifiedLinearConstraint less(l);
        ReifiedLinearConstraint more(l);
        if (!s_.isFalse(l.v) && (impl & Direction::FWD))
        {
            Literal x = s_.createNewLiteral();
            less.v = x;
            less.l.setRelation(LinearConstraint::Relation::LT);
            insert.emplace_back(std::move(less));
            Literal y = s_.createNewLiteral();
            more.v = y;
            more.l.setRelation(LinearConstraint::Relation::GT);
            insert.emplace_back(std::move(more));
            if (!s_.createClause(
                    LitVec{-x, orig})) /// having not orig implies not x (having x implies orig)
                return false;
            if (!s_.createClause(
                    LitVec{-y, orig})) /// having not orig implies not y (having y implies orig)
                return false;
            if (!s_.createClause(LitVec{-x, -y})) /// cant be less and more at same time
                return false;
            if (!s_.createClause(LitVec{x, y, -orig})) /// orig -> x or y
                return false;
        }
        if (!s_.isTrue(orig) && (impl & Direction::BACK))
        {
            l.v = -l.v;
            l.l.setRelation(LinearConstraint::Relation::LE);
            insert.emplace_back(std::move(l));
            u.v = -u.v;
            u.l.setRelation(LinearConstraint::Relation::GE);
            insert.emplace_back(std::move(u));
        }
    }
    return true;
}


std::pair< bool, bool > Normalizer::deriveSimpleDomain(ReifiedDomainConstraint &d)
{
    View v = d.getView();
    if ((s_.isFalse(d.getLiteral()) && d.getDirection() == Direction::FWD) ||
        (s_.isTrue(d.getLiteral()) && d.getDirection() == Direction::BACK))
        return std::make_pair(true, true);
    if (v.a == 0)
    {
        if (d.getDomain().in(v.c))
        {
            if (d.getDirection() == Direction::EQ)
                return std::make_pair(true, s_.setEqual(d.getLiteral(), s_.trueLit()));
            else
                return std::make_pair(true, true);
        }
        else
            return std::make_pair(true, s_.setEqual(d.getLiteral(), s_.falseLit()));
    }
    if (s_.isTrue(d.getLiteral()) && d.getDirection() & Direction::FWD)
    {
        if (!vc_.intersectView(d.getView(), d.getDomain())) return std::make_pair(true, false);
        return std::make_pair(true, true);
    }
    else if (s_.isFalse(d.getLiteral()) && d.getDirection() & Direction::BACK)
    {
        Domain all;
        all.remove(d.getDomain());
        if (!vc_.intersectView(d.getView(), all)) return std::make_pair(true, false);
        return std::make_pair(true, true);
    }
    return std::make_pair(false, true);
}

std::pair< bool, bool > Normalizer::deriveSimpleDomain(const ReifiedLinearConstraint &l)
{
    if ((s_.isFalse(l.v) && l.impl == Direction::FWD) ||
        (s_.isTrue(l.v) && l.impl == Direction::BACK))
        return std::make_pair(true, true);

    if (l.l.getViews().size() > 1) return std::make_pair(false, true);
    if (l.l.getViews().size() == 0)
    {
        bool failed = ((l.l.getRelation() == LinearConstraint::Relation::LE && 0 > l.l.getRhs()) ||
                       (l.l.getRelation() == LinearConstraint::Relation::EQ && 0 != l.l.getRhs()) ||
                       (l.l.getRelation() == LinearConstraint::Relation::NE && 0 == l.l.getRhs()));
        if (failed && (l.impl & Direction::FWD))
            return std::make_pair(true, s_.createClause(LitVec{-l.v}));
        if (!failed && (l.impl & Direction::BACK))
            return std::make_pair(true, s_.createClause(LitVec{l.v}));
        else
            return std::make_pair(true, true);
    }
    auto &view = l.l.getViews().front();
    if (l.l.getRelation() == LinearConstraint::Relation::LE)
    {
        if (s_.isTrue(l.v) && l.impl & Direction::FWD)
        {
            return std::make_pair(true, vc_.constrainUpperBound(view, l.l.getRhs()));
        }
        else if (s_.isFalse(l.v) && l.impl & Direction::BACK)
        {
            return std::make_pair(true, vc_.constrainLowerBound(view, l.l.getRhs() + 1));
        }
    }
    else
    {
        if (l.l.getRelation() == LinearConstraint::Relation::EQ)
        {
            if (s_.isTrue(l.v))
                return std::make_pair(true, vc_.constrainView(view, l.l.getRhs(), l.l.getRhs()));
            if (s_.isFalse(l.v)) /// not impl
                return std::make_pair(true, vc_.removeFromView(view, l.l.getRhs()));
        }
        else if (l.l.getRelation() == LinearConstraint::Relation::NE)
        {
            if (s_.isTrue(l.v)) return std::make_pair(true, vc_.removeFromView(view, l.l.getRhs()));
            if (s_.isFalse(l.v)) /// not impl
                return std::make_pair(true, vc_.constrainView(view, l.l.getRhs(), l.l.getRhs()));
        }
    }
    return std::make_pair(false, true);
}


bool Normalizer::addDistinct(ReifiedAllDistinct &&l)
{
    if (conf_.alldistinctCard) return addDistinctCardinality(std::move(l));
    // if (conf_.hallsize==0)
    return addDistinctPairwiseUnequal(std::move(l));
    // else
    //    return addDistinctHallIntervals(std::move(l));
}

/// size is sum of sizes of views
ViewDomain unify(const std::vector< View > &views, const VariableCreator &vc, uint64 &size)
{
    auto it = views.begin();
    // createOrderLiterals(*it);
    ViewDomain d(
        vc.getViewDomain(*(it++))); // yes, explicitly use domain of the view, not of the variable
    size += d.size();
    for (; it != views.end(); ++it)
    {
        // createOrderLiterals(*it);
        ViewDomain dd(vc.getViewDomain(*it));
        size += dd.size();
        d.unify(dd);
    }
    return d;
}


bool Normalizer::addDistinctCardinality(ReifiedAllDistinct &&l)
{
    auto &views = l.getViews();
    if (views.size() == 0) return true;

    uint64 size = 0;
    ViewDomain d = unify(views, vc_, size);

    if (views.size() > d.size())
    {
        if (!s_.createClause(LitVec{-l.getLiteral()})) return false;
        return true;
    }
    if (size == d.size()) // no overlap between variables
    {
        if ((l.getDirection() & Direction::BACK) && !s_.createClause(LitVec{l.getLiteral()}))
            return false;
        return true;
    }


    LitVec conditions;
    for (auto i : d)
    {
        LitVec lits;

        for (auto v : views) lits.emplace_back(vc_.getEqualLit(v, i));

        Literal x(0);
        if (s_.isTrue(l.getLiteral()))
            x = s_.falseLit();
        else
            x = s_.createNewLiteral();

        conditions.emplace_back(x);
        if (!s_.createCardinality(conditions.back(), 2, std::move(lits))) return false;
        if (l.getDirection() & Direction::FWD)
            if (!s_.createClause(LitVec{-conditions.back(), -l.getLiteral()})) return false;
    }

    conditions.emplace_back(l.getLiteral());
    if (l.getDirection() == Direction::FWD) return true;
    return s_.createClause(conditions);
}

bool Normalizer::addDistinctPairwiseUnequal(ReifiedAllDistinct &&l)
{
    auto &views = l.getViews();
    if (views.size() == 1) return true;

    uint64 size = 0;
    ViewDomain d = unify(views, vc_, size);

    if (views.size() > d.size())
    {
        if (!s_.createClause(LitVec{-l.getLiteral()})) return false;
        return true;
    }
    if (size == d.size()) // no overlap between variables
    {
        if ((l.getDirection() & Direction::BACK) && !s_.createClause(LitVec{l.getLiteral()}))
            return false;
        return true;
    }

    std::vector< LinearConstraint > inequalities;
    for (auto i = views.begin(); i != views.end() - 1; ++i)
        for (auto j = i + 1; j != views.end(); ++j)
        {
            LinearConstraint temp(LinearConstraint::Relation::NE);
            temp.add(*i);
            temp.add((*j) * -1); /// reverse view
            inequalities.emplace_back(temp);
        }

    LitVec lits;
    for (auto &i : inequalities)
    {
        /// in case of an implication, l can be used for all inequalities
        Literal x(0);
        if (s_.isTrue(l.getLiteral()))
            x = s_.trueLit();
        else
        {
            if (l.getDirection() == Direction::FWD)
                x = l.getLiteral();
            else
                x = s_.createNewLiteral();
        }
        std::vector< ReifiedLinearConstraint > tempv;
        if (!convertLinear(ReifiedLinearConstraint(LinearConstraint(i), x, l.getDirection()),
                           tempv))
            return false;
        else
        {
            linearConstraints_.insert(linearConstraints_.end(), tempv.begin(), tempv.end());
        }

        if (l.getDirection() & Direction::BACK)
        {
            lits.emplace_back(-x);
            if (!s_.createClause(LitVec{-l.getLiteral(), x})) return false;
        }
    }


    if (l.getDirection() & Direction::BACK)
    {
        /// if l.v is false, then at least one of the inequalities is also false
        lits.emplace_back(l.getLiteral());
        return s_.createClause(lits);
    }
    return true;
}


bool Normalizer::addDomainConstraint(ReifiedDomainConstraint &&d)
{
    LitVec longc;
    assert(!s_.isFalse(d.getLiteral()) && !s_.isTrue(d.getLiteral()));
    for (const auto &i : d.getDomain().getRanges())
    {
        if (i.l == i.u)
        {
            Literal u = vc_.getEqualLit(d.getView(), i.l);
            longc.emplace_back(u);
        }
        else
        {
            Literal u = s_.createNewLiteral();
            longc.emplace_back(u);

            Restrictor r = vc_.getRestrictor(d.getView());
            auto it = wrap_lower_bound(r.begin(), r.end(), i.l);
            Literal x = vc_.getGELiteral(it);
            it = wrap_upper_bound(it, r.end(), i.u);
            Literal y = vc_.getLELiteral(it - 1);

            if (!s_.createClause(LitVec{-u, x})) return false;
            if (!s_.createClause(LitVec{-u, y})) return false;
            if (!s_.createClause(LitVec{u, -x, -y})) return false;
        }
    }
    if (d.getDirection() & Direction::BACK)
    {
        for (const auto &i : longc)
            if (!s_.createClause(LitVec{-i, d.getLiteral()})) return false;
    }

    if (d.getDirection() & Direction::FWD)
    {
        longc.emplace_back(-d.getLiteral());
        if (!s_.createClause(longc)) return false;
    }

    return true;
}

bool Normalizer::calculateDomains()
{
    size_t removed = 0;
    for (size_t i = 0; i < domainConstraints_.size() - removed;)
    {
        auto &d = domainConstraints_[i];
        auto p = deriveSimpleDomain(d);
        if (!p.second) return false; // empty domain
        if (p.first)                 // simplified away
        {
            ++removed;
            domainConstraints_[i] =
                std::move(domainConstraints_[domainConstraints_.size() - removed]);
        }
        else
            ++i;
    }
    domainConstraints_.erase(domainConstraints_.begin() + (domainConstraints_.size() - removed),
                             domainConstraints_.end());


    removed = 0;
    for (size_t i = 0; i < linearConstraints_.size() - removed;)
    {
        auto &d = linearConstraints_[i];
        d.normalize();
        auto p = deriveSimpleDomain(d);
        if (!p.second) return false; // empty domain
        if (p.first)                 // simplified away
        {
            ++removed;
            linearConstraints_[i] =
                std::move(linearConstraints_[linearConstraints_.size() - removed]);
        }
        else
            ++i;
    }
    linearConstraints_.erase(linearConstraints_.begin() + (linearConstraints_.size() - removed),
                             linearConstraints_.end());


    removed = 0;
    for (size_t i = 0; i < this->allDistincts_.size() - removed;)
    {
        if ((allDistincts_[i].getDirection() == Direction::FWD &&
             s_.isFalse(allDistincts_[i].getLiteral())) ||
            (allDistincts_[i].getDirection() == Direction::BACK &&
             s_.isTrue(allDistincts_[i].getLiteral())))
        {
            ++removed;
            allDistincts_[i] = std::move(allDistincts_[allDistincts_.size() - removed]);
        }
        else
            ++i;
    }
    allDistincts_.erase(allDistincts_.begin() + (allDistincts_.size() - removed),
                        allDistincts_.end());

    return true;
}

namespace
{
    uint64 allLiterals(Variable v, const VariableCreator &vc)
    {
        return std::max(vc.getDomainSize(v), 1u) - 1;
    }
}

bool Normalizer::prepare()
{
    if (firstRun_)
    {
        varsBefore_ = 0;
        varsAfter_ = vc_.numVariables();
    }
    else
    {
        varsBefore_ = varsAfterFinalize_;
        varsAfter_ = vc_.numVariables();
    }

    /// calculate very first domains for easy constraints and remove them
    if (!calculateDomains()) return false;

    /// do propagation on true/false literals
    LinearPropagator p(s_, vc_, conf_);
    for (auto &i : linearConstraints_)
    {
        i.l.normalize();
        if (!s_.isTrue(i.v) && !s_.isFalse(i.v)) continue;
        if (i.l.getRelation() == LinearConstraint::Relation::LE)
        {
            ReifiedLinearConstraint l(i); // make a copy

            if (s_.isTrue(l.v) && l.impl & Direction::FWD)
            {
                p.addImp(std::move(l));
            }
            else if (s_.isFalse(l.v) && l.impl & Direction::BACK)
            {
                l.reverse();
                l.v = -l.v;
                p.addImp(std::move(l));
            }
        }
        if (i.l.getRelation() == LinearConstraint::Relation::EQ && s_.isTrue(i.v) &&
            i.impl == Direction::FWD)
        {
            ReifiedLinearConstraint l(i); // make a copy
            l.l.setRelation(LinearConstraint::Relation::LE);
            p.addImp(std::move(l));
            ReifiedLinearConstraint u(i); // make a copy
            u.l.setRelation(LinearConstraint::Relation::GE);
            p.addImp(std::move(u));
        }
    }
    if (!p.propagate()) return false;

    /// update the domain
    for (std::size_t i = 0; i < vc_.numVariables(); ++i)
    {
        if (getVariableCreator().isValid(i))
        {
            auto r = p.getVariableStorage().getCurrentRestrictor(i);
            vc_.constrainView(View(i), r.lower(),
                              r.upper()); /// should be the same as intersect (but cheaper),
        }
        /// as i only do bound propagation
    }

    firstRun_ = false;

    return auxprepare();
}

void Normalizer::addMinimize()
{
    for (auto p : minimize_)
    {
        View v = p.first;
        unsigned int level = p.second;
        auto &vc = getVariableCreator();
        auto res = vc.getRestrictor(v);
        uint64 before = 0;
        for (auto it = res.begin(); it != res.end(); ++it)
        {
            int32 w = ((*it) - before);
            before = *it;
            s_.addMinimize(vc.getGELiteral(it), w, level);
        }
    }
    minimize_.clear();
}


bool Normalizer::auxprepare()
{
    //    std::cout << "prepare" << std::endl;
    //    for (auto& i : linearConstraints_)
    //        std::cout << "Constraint " << i.l << " with rep " << i.v.asUint() << " is " <<
    //        s_.isFalse(i.v) << " " << s_.isTrue(i.v) << std::endl;

    // for (std::size_t i = 0; i < linearConstraints_.size(); ++i)
    //    std::cout << ":\t" << linearConstraints_[i].l << " << " <<
    //    linearConstraints_[i].v.asUint() << " true/false " << s_.isTrue(linearConstraints_[i].v)
    //    << "/" << s_.isFalse(linearConstraints_[i].v) <<std::endl;

    /// 1st: add all linear constraints
    std::vector< ReifiedLinearConstraint > tempv;
    unsigned int end = linearConstraints_.size();
    for (unsigned int i = 0; i < end;)
    {
        if (!convertLinear(std::move(linearConstraints_[i]), tempv)) /// adds it to the constraints
            return false;
        else
        {
            if (tempv.size())
            {
                tempv.front().normalize();
                tempv.front().l.sort(vc_, conf_);
                linearConstraints_[i] = std::move(tempv.front());
                for (auto j = tempv.begin() + 1; j != tempv.end(); ++j)
                {
                    j->normalize();
                    j->l.sort(vc_, conf_);
                    linearConstraints_.emplace_back(std::move(*j));
                }
                tempv.clear();
                ++i;
            }
            else
            {
                linearConstraints_[i] = std::move(linearConstraints_.back());
                linearConstraints_.pop_back();
                if (end < linearConstraints_.size() + 1)
                    ++i;
                else
                    --end;
            }
        }
    }
    for (auto &i : domainConstraints_)
    {
        if (!addDomainConstraint(std::move(i))) return false;
    }
    domainConstraints_.clear();
    // for (std::size_t i = 0; i < linearImplications_.size(); ++i)
    //    std::cout << i << ":\t" << linearImplications_[i].l << " << " <<
    //    linearImplications_[i].v.asUint() << std::endl;

    /// add even more constraints
    for (auto &i : allDistincts_)
        if (!addDistinct(std::move(i))) return false;
    allDistincts_.clear();

    /// remove 0sized linear constraints
    auto size = linearConstraints_.size();
    for (unsigned int i = 0; i < size;)
    {
        auto &l = linearConstraints_[i];
        l.normalize();
        if (l.l.getConstViews().size() == 0)
        {
            if (0 <= l.l.getRhs()) /// linear constraint  is satisfied
            {
                if (l.impl & Direction::BACK)
                    if (!s_.setEqual(l.v, s_.trueLit())) return false;
            }
            else /// linear constraint cant be satisfied
            {
                if (l.impl & Direction::FWD)
                {
                    if (!s_.setEqual(l.v, s_.falseLit())) return false;
                }
            }
            /// always remove
            --size;
            std::swap(l, linearConstraints_[size]);
        }
        else
            ++i;
    }

    linearConstraints_.erase(linearConstraints_.end() - (linearConstraints_.size() - size),
                             linearConstraints_.end());

    /// remove duplicates
    sort(linearConstraints_.begin(), linearConstraints_.end(),
         ReifiedLinearConstraint::compareless);
    linearConstraints_.erase(unique(linearConstraints_.begin(), linearConstraints_.end(),
                                    ReifiedLinearConstraint::compareequal),
                             linearConstraints_.end());


    // TODO do propagate only if added allDistinct constraints
    /// Cant do propagation here, as it does not propagate to clasp -> can restrict domains
    /// -> this can result in unused orderLits -> make them false in createOrderLits
    /// So either not propagate -> more order lits
    /// propagate and add clauses for clasp manually that apply after creating order literals
    /// cant use propagateWithReason as it uses orderLiterals

    /// TODO: do restrict variables if equality preprocessing restricts integer variables
    if (!vc_.restrictDomainsAccordingToLiterals()) return false;

    propagator_.reset(new LinearPropagator(s_, vc_, conf_));
    propagator_->addImp(std::move(linearConstraints_));
    linearConstraints_.clear();

    return propagate();
}

bool Normalizer::propagate()
{
    if (!vc_.restrictDomainsAccordingToLiterals()) return false;
    auto temp = std::move(propagator_->removeConstraints());
    propagator_.reset(new LinearPropagator(s_, vc_, conf_));
    propagator_->addImp(std::move(temp));

    // do propagate all original constraints
    if (!propagator_->propagate())
    {
        return false;
    }
    /// update the domain
    for (std::size_t i = 0; i < vc_.numVariables(); ++i)
    {
        if (getVariableCreator().isValid(i))
        {
            const auto &r = propagator_->getVariableStorage().getCurrentRestrictor(i);
            if (!vc_.constrainView(View(i), r.lower(), r.upper())) return false;
        }
    }
    return true;
}


bool Normalizer::finalize()
{
    linearConstraints_ = propagator_->removeConstraints();
    propagator_.reset();

    //    std::cout << "finalize" << std::endl;
    //    for (auto& i : linearConstraints_)
    //        std::cout << "Constraint " << i.l << " with rep " << i.v.asUint() << " is " <<
    //        s_.isFalse(i.v) << " " << s_.isTrue(i.v) << std::endl;

    vc_.prepareOrderLitMemory();

    if (!createEqualClauses()) return false;

    if (!vc_.createOrderLiterals()) return false;

    if (!translate(s_, getVariableCreator(), constraints(), conf_)) return false;


    /// what to do about it TODO
    /// if (conf_.explicitBinaryOrderClausesIfPossible && !createOrderClauses())
    ///    return false;


    addMinimize();

    // for (std::size_t i = 0; i < constraints_.size(); ++i)
    //    std::cout << i << ":\t" << constraints_[i].l << " << " << constraints_[i].v.asUint() <<
    //    std::endl;

    assert(allDistincts_.size() == 0);
    assert(domainConstraints_.size() == 0);
    assert(minimize_.size() == 0);

    varsAfterFinalize_ = vc_.numVariables();

    return true;
}


void Normalizer::variablesWithoutBounds(std::vector< Variable > &lb, std::vector< Variable > &ub)
{
    for (unsigned int i = varsBefore_; i < varsAfter_; ++i)
    {
        if (vc_.isValid(i))
        {
            if (vc_.getDomain(i).lower() == Domain::min) lb.push_back(i);
            if (vc_.getDomain(i).upper() == Domain::max) ub.push_back(i);
        }
    }
}


bool Normalizer::createOrderClauses()
{
    /// how to decide this incrementally?  recreate per var ?
    for (Variable var = 0; var < vc_.numVariables(); ++var)
    {
        if (getVariableCreator().isValid(var))
        {
            const auto &lr = vc_.getRestrictor(View(var));
            if (lr.size() >= 3)
            {
                auto start = pure_LELiteral_iterator(lr.begin(), vc_.getStorage(var), true);
                auto end = pure_LELiteral_iterator(lr.end() - 2, vc_.getStorage(var), false);
                if (end.isValid())
                    for (auto next = start; next != end;)
                    {
                        auto old = next;
                        ++next;
                        if (old.isValid() && next.isValid() &&
                            old.numElement() + 1 == next.numElement())
                            if (!s_.createClause(LitVec{-(*old), *next})) return false;
                        // if (vc_.hasLELiteral(v) && vc_.hasLELiteral(v+1))
                        // if
                        // (!s_.createClause(LitVec{-(vc_.getLELiteral(v)),vc_.getLELiteral(v+1)}))//
                        // could use GELiteral instead
                        //    return false;
                        //++v;
                    }
            }
        }
    }
    return true;
}

bool Normalizer::createEqualClauses() { return vc_.createEqualClauses(); }
}
