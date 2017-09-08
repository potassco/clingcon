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

#include <clingcon/constraint.h>
#include <clingcon/helper.h>
#include <clingcon/types.h>

namespace clingcon
{

void LinearConstraint::times(int32 x)
{
    for (auto &i : views_) i *= x;
    constant_ *= x;
    if (x < 0) invert();
}

bool LinearConstraint::productOfDomainsExceptLastLEx(const VariableCreator &vc, int64 x) const
{
    // assert(views_.size()!=0);
    assert(normalized_);
    if (x < 0) return true;
    uint64 ret = 1;
    for (std::size_t i = 0; i < views_.size() - 1; ++i)
    {
        ret *= vc.getDomainSize(views_[i]);
        if (ret > uint64(x)) return false;
    }
    return true;
}

uint64 LinearConstraint::productOfDomainsExceptLast(const VariableCreator &vc) const
{
    assert(views_.size() != 0);
    assert(normalized_);
    uint64 ret = 1;
    for (std::size_t i = 0; i < views_.size() - 1; ++i)
    {
        ret *= vc.getDomainSize(views_[i]);
    }
    return ret;
}

int LinearConstraint::normalize()
{
    if (normalized_) return 1;
    normalized_ = true;

    for (auto &i : views_)
    {
        constant_ -= i.c;
        i.c = 0;
    }

    switch (getRelation())
    {
    case LinearConstraint::Relation::LT:
        setRelation(LinearConstraint::Relation::LE);
        --constant_;
        break;
    case LinearConstraint::Relation::LE:
        break;
    case LinearConstraint::Relation::GT:
        ++constant_;
    case LinearConstraint::Relation::GE:
    {
        for (auto &i : views_) i.a *= -1;
        constant_ *= -1;
        setRelation(LinearConstraint::Relation::LE);
        break;
    }
    case LinearConstraint::Relation::EQ:
    case LinearConstraint::Relation::NE:
        break;
    default:
    {
        assert(false);
    }
    }

    if (views_.size() == 0) return 1;
    std::sort(views_.begin(), views_.end(), [](const View &x, const View &y) { return x.v < y.v; });

    /// had a look at reference implementation of unique
    auto result = views_.begin();
    auto first = result;
    while (++first != views_.end())
    {
        if (result->a == 0)
            *result = *first;
        else if (result->v != first->v)
            *(++result) = *first;
        else
            result->a += first->a;
    }
    if (result->a == 0)
        views_.erase(result, views_.end());
    else
        views_.erase(++result, views_.end());
    return factorize();
}

void LinearConstraint::reverse()
{
    switch (r_)
    {
    case LinearConstraint::Relation::EQ:
        r_ = LinearConstraint::Relation::NE;
        break;
    case LinearConstraint::Relation::NE:
        r_ = LinearConstraint::Relation::EQ;
        break;
    case LinearConstraint::Relation::LT:
        r_ = LinearConstraint::Relation::GE;
        break;
    case LinearConstraint::Relation::LE:
        r_ = LinearConstraint::Relation::GT;
        break;
    case LinearConstraint::Relation::GT:
        r_ = LinearConstraint::Relation::LE;
        break;
    case LinearConstraint::Relation::GE:
        r_ = LinearConstraint::Relation::LT;
        break;
    }
}

void LinearConstraint::invert()
{
    switch (r_)
    {
    case LinearConstraint::Relation::EQ:
        r_ = LinearConstraint::Relation::EQ;
        break;
    case LinearConstraint::Relation::NE:
        r_ = LinearConstraint::Relation::NE;
        break;
    case LinearConstraint::Relation::LT:
        r_ = LinearConstraint::Relation::GT;
        break;
    case LinearConstraint::Relation::LE:
        r_ = LinearConstraint::Relation::GE;
        break;
    case LinearConstraint::Relation::GT:
        r_ = LinearConstraint::Relation::LT;
        break;
    case LinearConstraint::Relation::GE:
        r_ = LinearConstraint::Relation::LE;
        break;
    }
}

int LinearConstraint::factorize()
{
    if (views_.size() == 0) return 1;
    int div = std::abs(views_.front().a);
    for (auto i : views_)
    {
        assert(i.c == 0);
        div = gcd(div, std::abs(i.a));
        if (div == 1) break;
    }
    if (constant_ != 0) div = gcd(div, constant_);

    if (div > 1)
    {
        for (auto &i : views_) i.a /= div;
        constant_ /= div;
    }
    return div;
}

void ReifiedLinearConstraint::normalize()
{
    switch (l.getRelation())
    {
    case LinearConstraint::Relation::EQ:
        break;
    case LinearConstraint::Relation::NE:
    {
        if (impl == Direction::EQ)
        {
            v = -v;
            l.setRelation(LinearConstraint::Relation::EQ);
        }
        break;
    }
    case LinearConstraint::Relation::LT:
        l.setRelation(LinearConstraint::Relation::LE);
        --l.constant_;
        break;
    case LinearConstraint::Relation::LE:
        break;
    case LinearConstraint::Relation::GT:
        ++l.constant_;
    case LinearConstraint::Relation::GE:
    {
        for (auto &i : l.views_)
        {
            i.a *= -1;
            i.c *= -1;
        }
        l.constant_ *= -1;
        l.setRelation(LinearConstraint::Relation::LE);
        break;
    }
    default:
        assert(false);
    }
    l.normalize();
}
}
