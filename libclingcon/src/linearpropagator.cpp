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

#include <clingcon/linearpropagator.h>

namespace clingcon
{

void ConstraintStorage::addImp(ReifiedLinearConstraint &&l)
{
    l.normalize();
    assert(l.l.getRelation() == LinearConstraint::Relation::LE);
    linearImpConstraints_.emplace_back(std::move(l));
    auto id = linearImpConstraints_.size() - 1;
    queueConstraint(id);
    for (auto i : l.l.getConstViews())
    {
        assert(i.a != 0);

        /// can sometimes add a constraint twice for the same variable, should not be a problem
        /// TODO: find a place to call unqiue ?
        if (!i.reversed())
        {
            lbChanges_.resize(std::max(i.v + 1, ( unsigned int )(lbChanges_.size())));
            lbChanges_[i.v].emplace_back(id);
        }
        else
        {
            ubChanges_.resize(std::max(i.v + 1, ( unsigned int )(ubChanges_.size())));
            ubChanges_[i.v].emplace_back(id);
        }
    }
}

void ConstraintStorage::addImp(const std::vector< ReifiedLinearConstraint > &vl)
{
    for (auto &l : vl) addImp(std::move(ReifiedLinearConstraint(l)));
}


void ConstraintStorage::addImp(std::vector< ReifiedLinearConstraint > &&vl)
{
    for (auto &l : vl) addImp(std::move(l));
}

void ConstraintStorage::removeLevel()
{
    for (auto i : toProcess_) linearImpConstraints_[i].l.setFlag(false);
    toProcess_.clear();
}


/// return false if the domain is empty
void ConstraintStorage::constrainUpperBound(const View &view, const BaseSolver &s)
{
    Variable v = view.v;

    if (ubChanges_.size() > v)
        for (auto i : ubChanges_[v])
        {
            bool check = (s.isTrue(linearImpConstraints_[i].v) && !view.reversed()) ||
                         (s.isFalse(linearImpConstraints_[i].v) && view.reversed());
            if (check || s.isUnknown(linearImpConstraints_[i].v)) queueConstraint(i);
        }

    if (lbChanges_.size() > v)
        for (auto i : lbChanges_[v])
        {
            bool check = (s.isTrue(linearImpConstraints_[i].v) && view.reversed()) ||
                         (s.isFalse(linearImpConstraints_[i].v) && !view.reversed());
            if (check || s.isUnknown(linearImpConstraints_[i].v)) queueConstraint(i);
        }
}

/// return false if the domain is empty
void ConstraintStorage::constrainLowerBound(const View &u, const BaseSolver &s)
{
    constrainUpperBound(u * -1, s);
}


/// remove all constraints,
/// moves the list of all reified implications out of the object
std::vector< ReifiedLinearConstraint > ConstraintStorage::removeConstraints()
{
    auto ret = std::move(linearImpConstraints_);
    linearImpConstraints_.clear();
    lbChanges_.clear();
    ubChanges_.clear();
    for (auto i : toProcess_) ret[i].l.setFlag(false);
    toProcess_.clear();
    return ret;
}


void ConstraintStorage::queueConstraint(std::size_t id)
{
    assert(id < linearImpConstraints_.size());
    if (!linearImpConstraints_[id].l.getFlag())
    {
        toProcess_.emplace_back(id);
        linearImpConstraints_[id].l.setFlag(true);
    }
}

std::size_t ConstraintStorage::popConstraint()
{
    size_t ret;
    ret = toProcess_.back();
    linearImpConstraints_[ret].l.setFlag(false);
    toProcess_.pop_back();
    return ret;
}


/// propagate, but not until a fixpoint
bool LinearPropagator::propagateSingleStep()
{
    if (!storage_.atFixPoint())
    {
        auto &lc = storage_.linearImpConstraints_[storage_.popConstraint()];
        if (s_.isTrue(lc.v))
        {
            if (!propagate_true(lc.l)) return false;
        }
        else if (s_.isUnknown(lc.v))
        {
            if (!propagate_impl(lc)) return false;
        }
    }
    return true;
}


/// return false if a domain gets empty
bool LinearPropagator::propagate()
{
    // process one of the list (others including itself maybe added), itself has to be removed
    // beforehand
    while (!storage_.atFixPoint())
    {
        if (!propagateSingleStep()) return false;
    }
    return true;
}

/// propagate, but not until a fixpoint
/// returns a set of new clauses
std::vector< LinearLiteralPropagator::LinearConstraintClause > &
LinearLiteralPropagator::propagateSingleStep(Solver &s)
{
    propClauses_.clear();
    while (!storage_.atFixPoint() && propClauses_.empty())
    {
        auto &lc = storage_.linearImpConstraints_[storage_.popConstraint()];
        if (s.isTrue(lc.v))
            propagate_true(lc, s);
        else if (conf_.propStrength >= 2 && s.isUnknown(lc.v))
            propagate_impl(lc);
    }
    return propClauses_;
}


std::pair< int64, int64 > LinearPropagator::computeMinMax(const LinearConstraint &l)
{
    auto &views = l.getViews();
    std::pair< int64, int64 > minmax(0, 0);
    for (auto &i : views)
    {
        auto r = vs_.getCurrentRestrictor(i);
        minmax.first += r.lower();
        minmax.second += r.upper();
    }
    return minmax;
}


std::pair< int64, int64 > LinearLiteralPropagator::computeMinMax(const LinearConstraint &l,
                                                                 itervec &clause)
{
    auto &views = l.getViews();
    std::pair< int64, int64 > minmax(0, 0);

    for (auto &i : views)
    {
        auto r = vs_.getVariableStorage().getCurrentRestrictor(i);
        assert(!r.isEmpty());
        minmax.first += r.lower();
        minmax.second += r.upper();
        clause.emplace_back(r.begin());
    }
    return minmax;
}

int64 LinearLiteralPropagator::computeMin(const LinearConstraint &l, itervec &clause)
{
    auto &views = l.getViews();
    int64 min(0);

    for (auto &i : views)
    {
        auto r = vs_.getVariableStorage().getCurrentRestrictor(i);
        assert(!r.isEmpty());
        min += r.lower();
        clause.emplace_back(r.begin());
    }
    return min;
}


bool LinearPropagator::propagate_true(const LinearConstraint &l)
{
    assert(l.getRelation() == LinearConstraint::Relation::LE);
    auto minmax = computeMinMax(l);
    if (minmax.second <= l.getRhs()) return true;

    for (auto &i : l.getViews())
    {
        auto r = vs_.getCurrentRestrictor(i);
        auto wholeRange = vs_.getRestrictor(i);
        int64 up = l.getRhs() - minmax.first + r.lower();
        if (up < r.lower())
        {
            // std::cout << "Constrain Variable " << i.first << " with new upper bound " << up <<
            // std::endl;
            return false;
        }
        if (up < r.upper())
        {
            // std::cout << "Constrain Variable " << i.first << " with new upper bound " << up <<
            // std::endl;
            // auto newUpper = std::lower_bound(wholeRange.begin(), wholeRange.end(), up);
            auto newUpper = wrap_upper_bound(wholeRange.begin(), r.end(), up);
            if (!constrainUpperBound(
                    newUpper)) // +1 is needed, as it is in iterator pointing after the element
                return false;
        }
    }
    return true;
}

void LinearLiteralPropagator::propagate_true(const ReifiedLinearConstraint &rl, const Solver &s)
{
    const LinearConstraint &l = rl.l;
    assert(l.getRelation() == LinearConstraint::Relation::LE);

    propClause_.clear();
    auto minmax = computeMinMax(l, propClause_);
    if (minmax.second <= l.getRhs()) return;

    if (conf_.propStrength <= 2)
    {
        if (minmax.first > l.getRhs())
        {
            propClauses_.emplace_back(std::make_pair(-rl.v, std::move(propClause_)));
        }
        return;
    }

    // std::cout << "trying to propagate " << l << std::endl;
    auto &views = l.getViews();
    for (std::size_t index = 0; index < views.size(); ++index)
    {
        auto &i = views[index];
        auto wholeRange = vs_.getVariableStorage().getRestrictor(i);
        assert(wholeRange.size() > 0);
        auto r = vs_.getVariableStorage().getCurrentRestrictor(i);
        assert(r.begin() < r.end());
        std::pair< int64, int64 > mm;
        mm.first = minmax.first - r.lower();
        mm.second = minmax.second - r.upper();

        // Literal prop = s_.falseLit();
        bool prop = false;
        Restrictor::ViewIterator propIt(wholeRange.begin());
        bool conflict = false;

        int64 up = l.getRhs() - mm.first;

        if (up < wholeRange.lower()) // derive false
        {
//            std::cout << "Constrain View " << i.v << "*" << i.a << "+" << i.c
//                      << " with new upper bound " << up << std::endl;
            // propIt=wholeRange.begin();//can be removed
            conflict = true;
        }
        else if (up < r.upper())
        {
//            std::cout << "Constrain Variable " << i.v << "*" << i.a << "+" << i.c
//                      << " with new upper bound " << up << std::endl;
//            std::cout << "This Variable before had domain " << r.lower() << " .. " << r.upper()
//                      << std::endl;
            auto newUpper = wrap_upper_bound(wholeRange.begin(), r.end(), up);
            // assert(newUpper != r.end()); /// should never be able to happen, as up <
            // r.upper().first, so there is something which is smaller, this means we do not need r
            // ?
            propIt = newUpper;
            if (newUpper == wholeRange.begin())
            {
                conflict = true;
            }
            else
            {
                --newUpper;
                prop = true;
                // prop = vs_.getVariableCreator().getLiteral(newUpper);
//                std::cout << "the upper bound not included for this view will be "
//                          << *(newUpper + 1) << std::endl;
                conflict = !constrainUpperBound(
                    (newUpper + 1),
                    s); // +1 is needed, as it is an iterator pointing after the element
                // minmax.first = mm.first + r.lower();
                minmax.second = mm.second + *newUpper;
                // minmax = mm +
                // std::minmax(i.second*(int64)r.lower(),i.second*(int64)((*newUpper)));
            }
        }

        if (prop || conflict)
        {
            itervec aux;
            if (conflict)
                aux = std::move(propClause_);
            else
                aux = propClause_;
            aux[index] = propIt;
            propClauses_.emplace_back(std::make_pair(-rl.v, std::move(aux)));
        }
        if (conflict) break;
    }
    // When i changed a bound, the reason for the next ones can change, right ? No! only the
    // upper/lower bound is changes, the other bound is used for reason
}


bool LinearPropagator::propagate_impl(ReifiedLinearConstraint &rl)
{
    const LinearConstraint &l = rl.l;
    assert(l.getRelation() == LinearConstraint::Relation::LE);
    auto minmax = computeMinMax(l);

    if (minmax.first > l.getRhs())
    {
        return s_.createClause(LitVec{-rl.v});
    }

    return true;
}


void LinearLiteralPropagator::propagate_impl(ReifiedLinearConstraint &rl)
{
    assert(conf_.propStrength >= 2);
    const LinearConstraint &l = rl.l;
    assert(l.getRelation() == LinearConstraint::Relation::LE);

    // std::cout << "trying to propagate_impl " << l << std::endl;
    propClause_.clear();
    auto min = computeMin(l, propClause_);
    if (min > l.getRhs())
    {
        /// shrink conflict
        if (conf_.propStrength >= 4)
        {
            // std::cout << "prop4" << std::endl;
            /// find a set of iterators such that the sum of it is minimally bigger than
            /// rl.l.getRhs()
            auto &views = l.getViews();
            std::size_t index = 0;
            while (index < views.size() && min - std::abs(views[index].a) > l.getRhs())
            {
                auto &i = views[index];
                auto wholeRange = vs_.getVariableStorage().getRestrictor(i);
                assert(wholeRange.size() > 0);
                auto r = vs_.getVariableStorage().getCurrentRestrictor(i);
                int64 mmin = min - r.lower();
                // mm.second = minmax.second - r.upper();

                int64 up = l.getRhs() - mmin;

                if (up < wholeRange.lower()) // derive false
                {
                    propClause_[index] = wholeRange.begin(); // false lit
                    min = mmin + *(wholeRange.begin());
                }
                else
                {
                    assert(up < r.upper());
                    auto newUpper = wrap_upper_bound(wholeRange.begin(), r.end(), up);
                    // assert(newUpper != r.end()); /// should never be able to happen, as up <
                    // r.upper().first, so there is something which is smaller, this means we do not
                    // need r ?
                    propClause_[index] = newUpper;
                    min = mmin + *newUpper;
                }
                ++index;
            }
        }
        propClauses_.emplace_back(-rl.v, std::move(propClause_));
        return;
    }
}


/// return false if the domain is empty
bool LinearPropagator::constrainUpperBound(const ViewIterator &u)
{
    storage_.constrainUpperBound(u.view(), s_);
    return vs_.constrainUpperBound(u);
}


/// return false if the domain is empty
bool LinearLiteralPropagator::constrainUpperBound(const ViewIterator &u, const BaseSolver &s)
{
    storage_.constrainUpperBound(u.view(), s);
    return vs_.getVariableStorage().constrainUpperBound(u);
}

/// return false if the domain is empty
bool LinearLiteralPropagator::constrainLowerBound(const ViewIterator &l, const BaseSolver &s)
{
    storage_.constrainLowerBound(l.view(), s);
    return vs_.getVariableStorage().constrainLowerBound(l);
}
} // namespace clingcon
