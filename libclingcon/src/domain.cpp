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

#include <clingcon/domain.h>


namespace clingcon
{

bool Domain::in(int32 x) const
{
    assert(!overflow_);
    auto it =
        std::lower_bound(ranges_.begin(), ranges_.end(), x, [](Range r, int x) { return r.u < x; });
    if (it == ranges_.end()) return false;
    return x >= it->l;
}

bool Domain::in(const Domain &x) const
{
    assert(!overflow_);
    for (auto i : x)
        if (!in(i)) return false;
    return true;
}


bool Domain::constrainDomain(int32 times, int32 c, int32 div)
{
    if ((times + c % div == 0) || abs(div) == 1) return true;

    Domain n(1, -1);

    if (div < ( int64 )size())
    {
        /// i can stop after 2 found numbers x,y, then y-x is the period
        /// of the new set to be created,
        /// Then I iterate over the ranges,
        /// and can calculate the rest
        int count = 0;
        int first, second;
        for (int i = 0; i != abs(div); ++i)
        {
            if ((i * times + c) % div == 0)
            {
                ++count;
                if (count == 1)
                    first = i;
                else if (count == 2)
                {
                    second = i;
                    break;
                }
            }
        }

        if (count > 0) // otherwise empty domain
        {
            int period;
            if (count == 1)
                period = abs(div);
            else
                period = second - first;

            for (auto &x : ranges_)
            {
                int start;
                int mod = x.l > 0 ? x.l % period : div - abs(x.l % period);
                if (first >= mod)
                    start = x.l + (first % period) - mod;
                else
                    start = x.l + (div - mod) + first;
                while (start <= x.u)
                {
                    n.ranges_.emplace_back(Range(start, start));
                    start += period;
                }
            }
        }
    }
    else
    {

        for (Domain::const_iterator i = begin(); i != end(); ++i)
            if ((times * (*i) + c) % div == 0) n.ranges_.emplace_back(Range(*i, *i));
    }
    n.ranges_.swap(ranges_);
    modified_ = true;
    return !empty();
}

bool Domain::intersect(int32 lower, int32 upper)
{
    modified_ = true;
    assert(!overflow_);
    if (lower > upper)
    {
        ranges_.clear();
        return false;
    }
    if (this->lower() == lower && this->upper() == upper) return true;
    assert(!empty());
    /// first one that contains the bound or is bigger
    auto it = std::lower_bound(ranges_.begin(), ranges_.end(), lower,
                               [](Range x, int lower) { return x.u < lower; });
    if (it == ranges_.end())
    {
        ranges_.clear();
        return false;
    }
    if (lower >= it->l) // inside the range
        it->l = lower;
    it = ranges_.erase(ranges_.begin(), it);
    /// first one that contains the bound or is bigger
    it = std::lower_bound(it, ranges_.end(), upper, [](Range x, int upper) { return x.u < upper; });
    if (it == ranges_.end()) return true;
    if (upper >= it->l) // inside the range
    {
        it->u = upper;
        ranges_.erase(it + 1, ranges_.end());
    }
    else
        ranges_.erase(it, ranges_.end());
    return !empty();
}


bool Domain::intersect(const Domain &d)
{
    modified_ = true;
    assert(!overflow_);
    assert(!empty());
    auto it = ranges_.begin();
    auto save = ranges_.begin();
    for (std::vector< Range >::const_iterator i = d.ranges_.begin(); i != d.ranges_.end(); ++i)
    {
        int lower = i->l;
        int upper = i->u;

        /// first one that contains the bound or is bigger
        it = std::lower_bound(it, ranges_.end(), lower,
                              [](Range x, int lower) { return x.u < lower; });
        if (it == ranges_.end())
        {
            ranges_.erase(save, ranges_.end());
            return !empty();
        }
        if (lower >= it->l) // inside the range
            it->l = lower;
        it = ranges_.erase(save, it);
        /// first one that contains the bound or is bigger
        it = std::lower_bound(it, ranges_.end(), upper,
                              [](Range x, int upper) { return x.u < upper; });
        if (it == ranges_.end()) return !empty();
        if (upper >= it->l) // inside the range
        {
            auto newlower = upper + 1;
            auto newupper = it->u;
            it->u = upper;
            if (i + 1 != d.ranges_.end())
                it = ranges_.insert(it + 1, Range(newlower, newupper));
            else
                ++it;
        }
        save = it;
    }
    ranges_.erase(save, ranges_.end());
    return !empty();
}


bool Domain::remove(int32 x)
{
    auto found = std::lower_bound(ranges_.begin(), ranges_.end(), x,
                                  [](const Range &r, int x) { return r.u < x; });
    if (found != ranges_.end())
    {
        if (x >= found->l)
        {
            modified_ = true;
            bool split = true;
            if (x == found->l)
            {
                ++(found->l);
                split = false;
            }
            if (x == found->u)
            {
                --(found->u);
                split = false;
            }
            if (found->l > found->u) ranges_.erase(found);
            if (split)
            {
                int oldu = found->u;
                found->u = x - 1;
                ranges_.insert(found + 1, Range(x + 1, oldu));
            }
            return !empty();
        }
        return !empty();
    }
    return !empty();
}

bool Domain::remove(int32 lower, int32 upper)
{
    assert(!overflow_);
    if (lower > upper) return true;
    assert(!empty());
    modified_ = true;
    Domain temp(*this);
    temp.intersect(temp.ranges_.front().l, lower - 1);

    intersect(upper + 1, ranges_.back().u);
    unify(temp);
    return !empty();
}

bool Domain::remove(const Domain &d)
{
    assert(!overflow_);
    for (const auto &i : d.ranges_)
        if (!remove(i.l, i.u)) return false;
    return true;
}


Domain &Domain::inplace_times(int32 n, int64 maxSize)
{
    assert(!overflow_);
    assert(n != 0);
    if (n < 0)
    {
        reverse();
        n *= -1;
    }
    if (n == 1) return *this;

    if (maxSize == -1 || (int64)(size()) <= (int64)(maxSize))
    {
        modified_ = true;
        std::vector< Range > temp;
        for (auto r : ranges_)
            for (int i = r.l; i <= r.u; ++i)
            {
                if (int64(i) * int64(n) > max || int64(i) * int64(n) < min)
                {
                    overflow_ = true;
                    return *this;
                }
                temp.push_back(Range(i * n, i * n));
            }
        swap(temp, ranges_);
    }
    else
    {
        modified_ = true;
        /// simple version, efficient but inaccurate
        for (auto &i : ranges_)
        {
            if (int64(i.l) * int64(n) < min || int64(i.u) * int64(n) > max)
            {
                overflow_ = true;
                return *this;
            }
            i.l *= n;
            i.u *= n;
        }
    }
    return *this;
}

Domain &Domain::inplace_divide(int32 n)
{
    assert(!overflow_);
    assert(n != 0);
    if (n < 0)
    {
        reverse();
        n *= -1;
    }
    if (n == 1) return *this;

    modified_ = true;
    Domain d;
    d.ranges_.clear();
    auto start = d.ranges_.end();

    for (auto &r : ranges_)
    {
        int32 old = -Domain::min - 1;
        for (int32 i = r.l; i <= r.u; ++i)
            if (i % n == 0 && (i / n != old))
            {
                start = d.add(start, Range(i / n, i / n));
                old = i / n;
            }
        //        int l = r.l <= 0 ? r.l + std::abs(r.l/n) : r.l - std::abs(r.l%n);
        //        int u = r.u <= 0 ? r.u + std::abs(r.u/n) : r.u - std::abs(r.u%n);
        //        if (l>u)
        //            continue;
    }

    d.ranges_.swap(ranges_);
    return *this;
}

void Domain::reverse()
{
    // reverse all ranges
    using std::swap;
    auto first = ranges_.begin();
    auto last = ranges_.end();
    while ((first != last) && (first != --last))
    {
        std::iter_swap(first, last);
        auto temp = first->l * -1;
        first->l = first->u * -1;
        first->u = temp;
        temp = last->l * -1;
        last->l = last->u * -1;
        last->u = temp;
        ++first;
    }
    if (first == last && ranges_.size() % 2 == 1)
    {
        auto temp = first->l * -1;
        first->l = first->u * -1;
        first->u = temp;
    }
}

Domain &Domain::operator+=(const Domain &d)
{
    assert(!overflow_);
    modified_ = true;
    if (d.size() == 1) return *this += d.lower();
    std::vector< Range > ret;
    using std::swap;
    swap(ret, ranges_);
    for (auto &i : ret)
    {
        for (auto j : d.ranges_)
        {
            if (int64(i.l) + int64(j.l) < min || int64(i.u) + int64(j.u) > max)
            {
                overflow_ = true;
                return *this;
            }
            unify(i.l + j.l, i.u + j.u);
        }
    }
    return *this;
}

Domain &Domain::operator+=(const int32 &d)
{
    assert(!overflow_);
    if (d == 0) return *this;
    modified_ = true;
    for (auto &i : ranges_)
    {
        if (int64(i.l) + int64(d) < min || int64(i.u) + int64(d) > max)
        {
            overflow_ = true;
            return *this;
        }
        i.l += d;
        i.u += d;
    }
    return *this;
}


std::vector< Range >::iterator Domain::add(std::vector< Range >::iterator start, const Range &r)
{
    assert(!overflow_);
    if (r.l > r.u) return start;
    modified_ = true;
    auto it = std::lower_bound(start, ranges_.end(), r, [](Range x, Range y) { return x.u < y.l; });
    /// we now have an iterator to an element which upper bound is bigger
    /// than our lower bound
    /// if it is at the end, we insert us there
    if (it == ranges_.end())
    {
        ranges_.push_back(r);
        it = ranges_.end() - 1;
    }
    else
    { /// it is actually really bigger
        if (it->l > r.u + 1)
        {
            it = ranges_.insert(it, r);
        }
        else
        {
            // the lower bound is extended
            if (it->l > r.l) it->l = r.l;

            // the lower bound is already included in the current(it) range
            if (r.u > it->u) // the upper bound is not yet included and can be expanded
            {
                it->u = r.u;
                auto upTo = it + 1;
                while (upTo != ranges_.end() && upTo->l <= r.u + 1)
                {
                    if (upTo->u > it->u) it->u = upTo->u;
                    ++upTo;
                }
                ranges_.erase(it + 1, upTo);
                // it is not invalid !
                if (r.u == it->u) ++it;
            }
        }
    }

    /// it was now already inserted, but it can happen that it can merge with
    /// a range below
    if (it != ranges_.begin() && (it - 1)->u + 1 == r.l)
    {
        (it - 1)->u = it->u;
        --it;
        ranges_.erase(it + 1);
    }
    return it;
}


Domain::const_iterator &Domain::const_iterator::operator++()
{
    assert(index_ < d_->ranges_.size());
    if (steps_ < (uint32)(d_->ranges_[index_].u - d_->ranges_[index_].l))
        ++steps_;
    else
    {
        ++index_;
        steps_ = 0;
    }
    assert(index_ < d_->ranges_.size() || (index_ == d_->ranges_.size() && steps_ == 0));
    return *this;
}

Domain::const_iterator &Domain::const_iterator::operator--()
{
    assert(index_ <= d_->ranges_.size());
    if (0 < steps_)
        --steps_;
    else
    {
        assert(index_ != 0);
        --index_;
        steps_ = d_->ranges_[index_].u - d_->ranges_[index_].l;
    }
    return *this;
}

bool Domain::const_iterator::operator<(const Domain::const_iterator &m) const
{
    if (index_ < m.index_) return true;
    if (index_ == m.index_) return steps_ < m.steps_;
    return false;
}

bool Domain::const_iterator::operator>(const Domain::const_iterator &m) const
{
    if (index_ > m.index_) return true;
    if (index_ == m.index_) return steps_ > m.steps_;
    return false;
}

int64 Domain::const_iterator::operator-(const Domain::const_iterator &m) const
{
    if (m > *this) return -(m - *this);
    // m <= this
    std::size_t count_index = m.index_;
    int64 count = -( int64 )m.steps_;
    while (count_index < index_)
    {
        count += (int64)(d_->ranges_[count_index].u) - d_->ranges_[count_index].l + 1;
        ++count_index;
    }
    count += steps_;
    return count;
}

Domain::const_iterator &Domain::const_iterator::operator+=(int64 x)
{
    if (x < 0) return operator-=(-x);
    assert(x <= std::numeric_limits< uint32 >::max());
    unsigned int add = (uint32)(x) + steps_;
    steps_ = 0;
    while (index_ < d_->ranges_.size())
    {
        uint64 range = (uint64)(d_->ranges_[index_].u) - d_->ranges_[index_].l;
        if (range < add)
        {
            ++index_;
            add -= range + 1;
        }
        else
        {
            steps_ += add;
            return *this;
        }
    }
    return *this;
}

Domain::const_iterator &Domain::const_iterator::operator-=(int64 x)
{
    x -= steps_;
    steps_ = 0;
    if (x <= 0) return operator+=(-x);
    assert(x <= std::numeric_limits< uint32 >::max());
    uint64 sub = (uint32)(x); /// sub > 0

    --index_;
    while (true)
    {
        uint64 range = (int64)(d_->ranges_[index_].u) - d_->ranges_[index_].l + 1;
        if (range < sub)
        {
            --index_;
            sub -= range;
        }
        else
        {
            steps_ = range - sub;
            return *this;
        }
    }
    assert(false);
}


/// ViewDomain
///
///
///
///

bool ViewDomain::in(int64 x) const
{
    auto it = std::lower_bound(ranges_.begin(), ranges_.end(), x,
                               [](LongRange r, int64 x) { return r.u < x; });
    if (it == ranges_.end()) return false;
    return x >= it->l;
}


std::vector< LongRange >::iterator ViewDomain::add(std::vector< LongRange >::iterator start,
                                                   const LongRange &r)
{
    if (r.l > r.u) return start;
    auto it = std::lower_bound(start, ranges_.end(), r,
                               [](LongRange x, LongRange y) { return x.u + 1 < y.l; });
    if (it == ranges_.end())
    {
        ranges_.push_back(r);
        return ranges_.end();
    }

    if (it->l > r.u + 1)
    {
        return ranges_.insert(it, r);
    }
    // the lower bound is extended
    if (it->l > r.l) it->l = r.l;
    // the lower bound is already included in the current range
    if (r.u > it->u) // the upper bound is not yet included and can be expanded
    {
        it->u = r.u;
        auto upTo = it + 1;
        while (upTo != ranges_.end() && upTo->l <= r.u + 1)
        {
            if (upTo->u > it->u) it->u = upTo->u;
            ++upTo;
        }
        ranges_.erase(it + 1, upTo);
        // it is not invalid !
        if (r.u == it->u)
            return it + 1;
        else
            return it;
    }
    return it;
}


ViewDomain::const_iterator &ViewDomain::const_iterator::operator++()
{
    assert(index_ < d_->ranges_.size());
    if (steps_ < (uint64)(d_->ranges_[index_].u - d_->ranges_[index_].l))
        ++steps_;
    else
    {
        ++index_;
        steps_ = 0;
    }
    assert(index_ < d_->ranges_.size() || (index_ == d_->ranges_.size() && steps_ == 0));
    return *this;
}

ViewDomain::const_iterator &ViewDomain::const_iterator::operator--()
{
    assert(index_ <= d_->ranges_.size());
    if (0 < steps_)
        --steps_;
    else
    {
        assert(index_ != 0);
        --index_;
        steps_ = d_->ranges_[index_].u - d_->ranges_[index_].l;
    }
    return *this;
}

bool ViewDomain::const_iterator::operator<(const ViewDomain::const_iterator &m) const
{
    if (index_ < m.index_) return true;
    if (index_ == m.index_) return steps_ < m.steps_;
    return false;
}

bool ViewDomain::const_iterator::operator>(const ViewDomain::const_iterator &m) const
{
    if (index_ > m.index_) return true;
    if (index_ == m.index_) return steps_ > m.steps_;
    return false;
}

int64 ViewDomain::const_iterator::operator-(const ViewDomain::const_iterator &m) const
{
    if (m > *this) return -(m - *this);
    // m <= this
    std::size_t count_index = m.index_;
    int64 count = -( int64 )m.steps_;
    while (count_index < index_)
    {
        count += (int64)(d_->ranges_[count_index].u) - d_->ranges_[count_index].l + 1;
        ++count_index;
    }
    count += steps_;
    return count;
}

/// can potentially overflow silently
ViewDomain::const_iterator &ViewDomain::const_iterator::operator+=(int64 x)
{
    if (x < 0) return operator-=(-x);
    assert(x <= std::numeric_limits< uint32 >::max());
    int64 add = (uint64)(x) + steps_;
    steps_ = 0;
    while (true)
    {
        uint64 range = (uint64)(d_->ranges_[index_].u) - d_->ranges_[index_].l;
        if (range < (uint64)(add))
        {
            ++index_;
            add -= range + 1;
        }
        else
        {
            steps_ += add;
            return *this;
        }
    }
    assert(false);
}

ViewDomain::const_iterator &ViewDomain::const_iterator::operator-=(int64 x)
{
    x -= steps_;
    steps_ = 0;
    if (x <= 0) return operator+=(-x);
    assert(x <= std::numeric_limits< uint32 >::max());
    uint64 sub = (uint64)(x); /// sub > 0

    --index_;
    while (true)
    {
        uint64 range = (int64)(d_->ranges_[index_].u) - d_->ranges_[index_].l + 1;
        if (range < sub)
        {
            --index_;
            sub -= range;
        }
        else
        {
            steps_ = range - sub;
            return *this;
        }
    }
    assert(false);
}


ViewIterator lower_bound(ViewIterator first, ViewIterator last, int64 value)
{
    if (first == last) return first;
    View view = first.view();
    assert(first.view() == last.view());
    ViewIterator it;
    int64 count, step;
    count = last.index_ - first.index_;

    int64 u = view.multiply(view.reversed() ? first.it_.d_->getRanges()[(first.it_ - 1).index_].l :
                                              first.it_.d_->getRanges()[first.it_.index_].u);
    if (value > u)
        while (count > 0)
        {
            /// if in current range (value<=u)

            it = first;
            int64 changer = view.reversed() ? (first.it_ - 1).steps_ :
                                              first.it_.d_->getRanges()[first.it_.index_].size() -
                                                  first.it_.steps_ - 1;
            step = std::max(count / 2, changer);
            it += step;
            if (*it < value)
            {
                ++it;
                first = it;
                if (it >= last) break;
                u = view.multiply(view.reversed() ?
                                      first.it_.d_->getRanges()[(first.it_ - 1).index_].l :
                                      first.it_.d_->getRanges()[first.it_.index_].u);
                if (value <= u) break;
                count -= step + 1;
            }
            else
                count = step;
        }
    /// now the iterator is at the right range

    if (first == last) return first;

    int64 diff = std::ceil(double(value - (*first)) / std::abs(view.a));
    if (diff > 0) first += diff;
    return first;
}


ViewIterator upper_bound(ViewIterator first, ViewIterator last, int64 value)
{
    auto it = lower_bound(first, last, value);
    if (it != last && *it == value) ++it;
    return it;
}
} // namespace clingcon
