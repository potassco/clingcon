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
#include <clingcon/solver.h>
#include <clingcon/types.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <limits>
#include <vector>

namespace clingcon
{

class Range
{
public:
    Range(int32 lower, int32 upper)
        : l(lower)
        , u(upper)
    {
    }
    bool operator==(const Range &d) const { return l == d.l && u == d.u; }
    bool operator!=(const Range &d) const { return l != d.l || u != d.u; }
    int32 l, u;
    int64 size() const { return (( int64 )u - ( int64 )l) + 1; }
};

static_assert(std::numeric_limits< int32 >::min() == -2147483648 &&
                  std::numeric_limits< int32 >::max() == 2147483647,
              "requires int to be 32bit and between -2147483648 .. 2147483647");

class Restrictor;
class ViewIterator;
class ViewDomain;
/****
  Domain Class
  Maximum Bounds: should be std::numeric_limits<int>::min()/2+2 and
std::numeric_limits<int>::max()/2-1
  Does overflow()==true if numbers get out of range
  Behaviour on calling a function on an overflown domain is undefined
  Can be empty
****/
class Domain
{
public:
    static const int32 min = std::numeric_limits< int32 >::min() / 2 + 2;
    static const int32 max = std::numeric_limits< int32 >::max() / 2 - 1;

    /// create a maximum size domain

    Domain(const Domain &d) = default;
    Domain(Domain &&) = default;

    Domain &operator=(const Domain &d) = default;
    Domain &operator=(Domain &&) = default;
    Domain()
        : ranges_{Range(min, max)}
        , size_(0)
        , modified_(true)
        , overflow_(false)
    {
    }
    /// creates a domain with the current restricted variable domain r
    // Domain(const Restrictor& r);// do i need exact domain or
    // approximation!??!
    /// shortcut for Domain(a,a)
    Domain(int32 a)
        : Domain(a, a)
    {
    }
    /// create a domain [lower..upper]
    Domain(int32 lower, int32 upper)
        : ranges_{Range(lower, upper)}
        , size_(0)
        , modified_(true)
        , overflow_(false)
    {
        if (lower > upper) ranges_.clear();
    }

    const std::vector< Range > &getRanges() const { return ranges_; }
    bool overflow() const { return overflow_; }

    /// print a domain, short notation
    friend std::ostream &operator<<(std::ostream &stream, const Domain &d);

    /// return true if x is in the domain
    bool in(int32 x) const;
    /// return true if x is in the domain
    /// TODO: can be optimized, currently only used in assertions
    bool in(const Domain &x) const;
    /// return the smallest element in the domain
    /// pre: !empty()
    int32 lower() const
    {
        assert(!empty());
        return ranges_.front().l;
    }
    /// return the biggest element in the domain
    /// pre: !empty()
    int32 upper() const
    {
        assert(!empty());
        return ranges_.back().u;
    }
    /// returns true if domain is empty, false otherwise
    bool empty() const { return ranges_.size() == 0 || ranges_.front().l > ranges_.back().u; }

    bool operator==(const Domain &d) const { return ranges_ == d.ranges_; }
    bool operator!=(const Domain &d) const { return ranges_ != d.ranges_; }

    Domain &inplace_times(int32 n, int64 maxSize);

    /// constrain all values i to fulfill: (times*i+c)%div==0
    /// return false if empty
    bool constrainDomain(int32 times, int32 c, int32 div);

    /// divides the domain elements by n
    /// pre: n != 0
    /// removes all elements a, where a mod 0 != 0 before dividing
    Domain &inplace_divide(int32 n);

    /// multiplies the domain elements by n if the size()
    /// is below a certain threshold, otherwise
    /// the bounds of the ranges are multiplied with n to get an
    /// overapproximation
    /// pre: n!= 0
    Domain times(int32 n, int64 maxSize) const
    {
        assert(n != 0);
        Domain ret(*this);
        ret.inplace_times(n, maxSize);
        return ret;
    }

    /// pairwise addition of two domains
    Domain operator+(const Domain &d) const
    {
        Domain ret(*this);
        ret += d;
        return ret;
    }

    /// pairwise addition of two domains
    /// TODO, maybe not optimal in runtime
    Domain &operator+=(const Domain &d);

    /// pairwise addition of two domains
    Domain operator+(const int32 &d) const
    {
        Domain ret(*this);
        ret += d;
        return ret;
    }

    /// pairwise addition of two domains
    /// TODO, maybe not optimal in runtime
    Domain &operator+=(const int32 &d);

    /// intersect the domain with d (can be empty)
    /// can also result in empty domain
    /// pre: domain is not empty
    /// return false if domain gets empty
    bool intersect(const Domain &d);

    /// intersect the domain with the Domain(lower,upper) (can be empty)
    /// can also result in empty domain
    /// pre: domain is not empty
    /// return false if domain gets empty
    bool intersect(int32 lower, int32 upper);

    /// remove the Domain(lower, upper) (can be empty) from the Domain
    /// pre: domain is not empty
    /// TODO: can be optimized
    /// return false if domain gets empty
    bool remove(const Domain &d);

    bool remove(int32 x);
    /// remove the Domain(lower, upper) (can be empty) from the Domain
    /// pre: domain is not empty
    /// return false if domain gets empty
    bool remove(int32 lower, int32 upper);

    /// unify domain with the Domain(lower,upper)
    void unify(int32 lower, int32 upper) { add(ranges_.begin(), Range(lower, upper)); }

    /// unify domain with the d
    /// can be empty
    void unify(const Domain &d)
    {
        if (empty())
        {
            ranges_ = d.ranges_;
            size_ = d.size_;
            modified_ = d.modified_;
            return;
        }
        auto start = ranges_.begin();
        for (auto i : d.ranges_) start = add(start, i);
    }

    /// returns the number of elements in the domain
    /// lazily evaluates and stores value
    uint64 size() const
    {
        if (modified_)
        {
            modified_ = false;
            size_ = 0;
            for (auto i : ranges_) size_ += (uint64)(((int64)(i.u) - (int64)(i.l)) + 1);
        }
        return size_;
    }

    class const_iterator;
    friend const_iterator;

    const_iterator begin() const { return const_iterator(this, 0, 0); }
    const_iterator end() const { return const_iterator(this, ranges_.size(), 0); }

    class const_iterator : public std::iterator< std::random_access_iterator_tag, int >
    {
    public:
        friend Domain;
        const_iterator() = default;
        const_iterator(const const_iterator &) = default;
        const_iterator &operator=(const const_iterator &) = default;
        // please do only compare iterators of the same domain
        bool operator==(const_iterator const &x) const
        {
            return index_ == x.index_ && steps_ == x.steps_;
        }
        bool operator!=(const_iterator const &x) const
        {
            return index_ != x.index_ || steps_ != x.steps_;
        }

        const_iterator &operator++();

        const_iterator operator++(int)
        {
            const_iterator tmp(*this);
            operator++();
            return tmp;
        }

        const_iterator &operator--();

        const_iterator operator--(int)
        {
            const_iterator tmp(*this);
            operator--();
            return tmp;
        }

        bool operator<(const const_iterator &m) const;

        bool operator>(const const_iterator &m) const;

        bool operator>=(const const_iterator &m) const { return !(*this < m); }
        bool operator<=(const const_iterator &m) const { return !(*this > m); }

        int64 operator-(const const_iterator &m) const;

        const_iterator &operator+=(int64 x);

        const_iterator &operator-=(int64 x);

        const_iterator operator+(int64 x) const
        {
            const_iterator tmp = *this;
            tmp += x;
            return tmp;
        }

        const_iterator operator-(int64 x) const
        {
            const_iterator tmp = *this;
            tmp -= x;
            return tmp;
        }

        const Domain &getDomain() const { return *d_; }

        friend ViewIterator lower_bound(ViewIterator, ViewIterator, int64);
        friend ViewIterator upper_bound(ViewIterator, ViewIterator, int64);

        int32 operator*() const
        {
            assert(index_ < d_->ranges_.size());
            return d_->ranges_[index_].l + int(steps_);
        }
        int32 operator->() const
        {
            assert(index_ < d_->ranges_.size());
            return d_->ranges_[index_].l + int(steps_);
        }

    private:
        const_iterator(Domain const *d, int index, int steps)
            : d_(d)
            , index_(index)
            , steps_(steps)
        {
        }
        Domain const *d_;
        std::size_t index_; /// index into the range vector
        uint32 steps_;      /// the number of steps to go from the current lower bound
    };

    friend ViewIterator;
    friend ViewDomain;

private:
    /// multiply with -1
    void reverse();

    /// unify the domain with the [range],
    /// return iterator to a range where a bigger range can be inserted next
    /// time
    std::vector< Range >::iterator add(std::vector< Range >::iterator start, const Range &r);

    std::vector< Range > ranges_;
    mutable uint64 size_;
    mutable bool modified_;
    bool overflow_;
};

inline std::ostream &operator<<(std::ostream &stream, const Domain &d)
{
    for (auto i = d.ranges_.begin(); i != d.ranges_.end(); ++i)
    {
        stream << i->l << ".." << i->u;
        if (i + 1 != d.ranges_.end()) stream << ",";
    }

    return stream;
}

class LongRange
{
public:
    LongRange(int64 lower, int64 upper)
        : l(lower)
        , u(upper)
    {
    }
    bool operator==(const LongRange &d) const { return l == d.l && u == d.u; }
    bool operator!=(const LongRange &d) const { return l != d.l || u != d.u; }
    int64 l, u;
};

/// special 64bit domain, created from Domain + View
/// cant overflow, and only has simple functionality, except unify
class ViewDomain
{
public:
    static const int64 min = std::numeric_limits< int64 >::min() + 2;
    static const int64 max = std::numeric_limits< int64 >::max() - 1;

    /// create a maximum size domain

    ViewDomain(const ViewDomain &d) = default;
    ViewDomain(ViewDomain &&) = default;

    ViewDomain &operator=(const ViewDomain &d) = default;
    ViewDomain &operator=(ViewDomain &&) = default;
    ViewDomain(int64 lower, int64 upper)
        : ranges_{LongRange(lower, upper)}
    {
    }
    ViewDomain(Domain d, View v)
    {
        int64 n = v.a;
        int64 c = v.c;

        if (n == 0)
        {
            ranges_.emplace_back(c, c);
            return;
        }

        if (n < 0)
        {
            d.reverse();
            n *= -1;
        }
        if (n == 1)
        {
            for (auto r : d.ranges_) ranges_.emplace_back(r.l + c, r.u + c);
            return;
        }

        for (auto r : d.ranges_)
            for (int64 i = r.l; i <= r.u; ++i)
            {
                ranges_.emplace_back(i * n + c, i * n + c);
            }
    }

    /// print a domain, short notation
    friend std::ostream &operator<<(std::ostream &stream, const ViewDomain &d);

    /// return true if x is in the domain
    bool in(int64 x) const;

    /// return the smallest element in the domain
    /// pre: !empty()
    int64 lower() const
    {
        assert(!empty());
        return ranges_.front().l;
    }
    /// return the biggest element in the domain
    /// pre: !empty()
    int64 upper() const
    {
        assert(!empty());
        return ranges_.back().u;
    }
    /// returns true if domain is empty, false otherwise
    bool empty() const { return ranges_.size() == 0 || ranges_.front().l > ranges_.back().u; }

    bool operator==(const ViewDomain &d) { return ranges_ == d.ranges_; }
    bool operator!=(const ViewDomain &d) { return ranges_ != d.ranges_; }

    /// unify domain with the Domain(lower,upper)
    void unify(int64 lower, int64 upper) { add(ranges_.begin(), LongRange(lower, upper)); }

    /// unify domain with the d
    /// can be empty
    void unify(const ViewDomain &d)
    {
        if (empty())
        {
            ranges_ = d.ranges_;
            return;
        }
        auto start = ranges_.begin();
        for (auto i : d.ranges_) start = add(start, i);
    }

    /// returns the number of elements in the domain
    uint64 size() const
    {
        uint64 size = 0;
        for (auto i : ranges_) size += (i.u - i.l) + 1;
        return size;
    }

    class const_iterator;
    friend const_iterator;

    const_iterator begin() const { return const_iterator(this, 0, 0); }
    const_iterator end() const { return const_iterator(this, ranges_.size(), 0); }

    class const_iterator : public std::iterator< std::random_access_iterator_tag, int64 >
    {
    public:
        friend ViewDomain;
        const_iterator() = default;
        const_iterator(const const_iterator &) = default;
        const_iterator &operator=(const const_iterator &) = default;
        // please do only compare iterators of the same domain
        bool operator==(const_iterator const &x) const
        {
            return index_ == x.index_ && steps_ == x.steps_;
        }
        bool operator!=(const_iterator const &x) const
        {
            return index_ != x.index_ || steps_ != x.steps_;
        }

        const_iterator &operator++();

        const_iterator operator++(int)
        {
            const_iterator tmp(*this);
            operator++();
            return tmp;
        }

        const_iterator &operator--();

        const_iterator operator--(int)
        {
            const_iterator tmp(*this);
            operator--();
            return tmp;
        }

        bool operator<(const const_iterator &m) const;

        bool operator>(const const_iterator &m) const;

        bool operator>=(const const_iterator &m) const { return !(*this < m); }
        bool operator<=(const const_iterator &m) const { return !(*this > m); }

        /// can this overflow ?
        int64 operator-(const const_iterator &m) const;

        const_iterator &operator+=(int64 x);

        const_iterator &operator-=(int64 x);

        const_iterator operator+(int64 x) const
        {
            const_iterator tmp = *this;
            tmp += x;
            return tmp;
        }

        const_iterator operator-(int64 x) const
        {
            const_iterator tmp = *this;
            tmp -= x;
            return tmp;
        }

        const ViewDomain &getDomain() const { return *d_; }

        int64 operator*() const
        {
            assert(index_ < d_->ranges_.size());
            return d_->ranges_[index_].l + int(steps_);
        }
        int64 operator->() const
        {
            assert(index_ < d_->ranges_.size());
            return d_->ranges_[index_].l + int(steps_);
        }

    private:
        const_iterator(ViewDomain const *d, int index, uint64 steps)
            : d_(d)
            , index_(index)
            , steps_(steps)
        {
        }
        ViewDomain const *d_;
        std::size_t index_; /// index into the range vector
        uint64 steps_;      /// the number of steps to go from the current lower bound
    };

private:
    /// unify the domain with the [range],
    /// return iterator to a range where a bigger range can be inserted next
    /// time
    std::vector< LongRange >::iterator add(std::vector< LongRange >::iterator start,
                                           const LongRange &r);

    std::vector< LongRange > ranges_;
};

inline std::ostream &operator<<(std::ostream &stream, const ViewDomain &d)
{
    for (auto i = d.ranges_.begin(); i != d.ranges_.end(); ++i)
    {
        stream << i->l << ".." << i->u;
        if (i + 1 != d.ranges_.end()) stream << ",";
    }

    return stream;
}

class Restrictor;

class ViewIterator : public std::iterator< std::random_access_iterator_tag, int >
{
public:
    friend Restrictor;

    ViewIterator() = default;

    ViewIterator(const ViewIterator &) = default;
    ViewIterator &operator=(const ViewIterator &) = default;

    View view() const { return v_; }
    // please do only compare iterators of the same view
    bool operator==(ViewIterator const &x) const
    {
        assert(v_ == x.v_);
        return index_ == x.index_;
    }
    bool operator!=(ViewIterator const &x) const
    {
        assert(v_ == x.v_);
        return index_ != x.index_;
    }

    ViewIterator &operator++()
    {
        v_.reversed() ? --it_ : ++it_;
        ++index_;
        return *this;
    }

    ViewIterator operator++(int)
    {
        ViewIterator tmp(*this);
        operator++();
        return tmp;
    }

    ViewIterator &operator--()
    {
        assert(index_ > 0);
        v_.reversed() ? ++it_ : --it_;
        --index_;
        return *this;
    }

    ViewIterator operator--(int)
    {
        ViewIterator tmp(*this);
        operator--();
        return tmp;
    }

    /// should not matter which iterator i use
    bool operator<(const ViewIterator &m) const { return index_ < m.index_; }

    bool operator>(const ViewIterator &m) const { return index_ > m.index_; }

    bool operator>=(const ViewIterator &m) const { return !(*this < m); }
    bool operator<=(const ViewIterator &m) const { return !(*this > m); }

    int64 operator-(const ViewIterator &m) const { return index_ - m.index_; }

    ViewIterator &operator+=(int64 x)
    {
        index_ += x;
        v_.reversed() ? it_ -= x : it_ += x;
        return *this;
    }

    ViewIterator &operator-=(int64 x)
    {
        index_ -= x;
        v_.reversed() ? it_ += x : it_ -= x;
        return *this;
    }

    ViewIterator operator+(int64 x) const
    {
        ViewIterator tmp = *this;
        tmp += x;
        return tmp;
    }

    ViewIterator operator-(int64 x) const
    {
        ViewIterator tmp = *this;
        tmp -= x;
        return tmp;
    }

    int64 operator*() const { return v_.multiply(v_.reversed() ? *(it_ - 1) : *it_); }
    int64 operator->() const { return v_.multiply(v_.reversed() ? *(it_ - 1) : *it_); }

    /// returns this - domain.begin()
    uint64 numElement() const { return index_; }

    /// convert an iterator from a view to a one for the simple variable
    /// (a=1,c=0)
    /// points to the same position in the domain
    static ViewIterator viewToVarIterator(const ViewIterator &it)
    {
        ViewIterator ret;
        ret.it_ = it.it_;
        ret.v_.v = it.v_.v;
        ret.v_.a = 1;
        ret.v_.c = 0;
        ret.index_ = it.index_;
        if (it.view().reversed())
        {
            if (it.it_.getDomain().size() == it.index_)       /// we are at the end
                ret.it_ = it.it_ + it.it_.getDomain().size(); // index is already set for end it
            else
            {
                --ret.it_;
                ret.index_ = it.it_.getDomain().size() - 1 - it.index_;
            }
        }

        return ret;
    }
    friend ViewIterator lower_bound(ViewIterator, ViewIterator, int64);
    friend ViewIterator upper_bound(ViewIterator, ViewIterator, int64);

    // const Domain& getDomain() const { return it_.getDomain()v_.a + v_.c_; }

private:
    ViewIterator(const View &v, const Domain::const_iterator &it, unsigned int index)
        : v_(v)
        , it_(it)
        , index_(index)
    {
    }
    View v_;
    Domain::const_iterator it_;
    uint64 index_; // index refers to the number of the current element,
                   // this-begin,
                   // so if view is reversed,
    // the iterator for the first element points to the end(and refers to the
    // last
    // element of the original domain)
    // and index_ is still 0 (1st element)
};

ViewIterator lower_bound(ViewIterator first, ViewIterator last, int64 value);
ViewIterator upper_bound(ViewIterator first, ViewIterator last, int64 value);

inline ViewIterator wrap_upper_bound(ViewIterator first, ViewIterator last, int64 value)
{
    assert(clingcon::upper_bound(first, last, value) == std::upper_bound(first, last, value));
    return clingcon::upper_bound(first, last, value);
}

inline ViewIterator wrap_lower_bound(ViewIterator first, ViewIterator last, int64 value)
{
    assert(lower_bound(first, last, value) == std::lower_bound(first, last, value));
    return clingcon::lower_bound(first, last, value);
}

/// TODO actually stores too much information, pointer to domain is stored
/// twice!
/// I have to reverse the iterators in case of v.reversed()
class Restrictor
{
public:
    using ViewIterator = clingcon::ViewIterator;
    Restrictor() = default;
    Restrictor(const View &v, const Domain &d)
        : lower_(v, v.reversed() ? d.end() : d.begin(), 0)
        , upper_(v, v.reversed() ? d.begin() : d.end(), d.size())
    {
    }
    Restrictor(const Restrictor &r) = default;

    /// change view on the restrictor, and also have a restricted version (does
    /// not point to start/end of domain but somewhere in between)
    /// numElement of the iterators still refers to the gobal domain
    /// pre: r.begin().view().v == v.v
    /// pre: r.begin().view().a = 1
    /// pre: r.begin().view().c = 0
    Restrictor(const View &v, const Restrictor &r)
        : lower_(v, v.reversed() ? r.end().it_ : r.begin().it_,
                 v.reversed() ? r.begin().it_.getDomain().size() - r.end().index_ :
                                r.begin().index_)
        , upper_(v, v.reversed() ? r.begin().it_ : r.end().it_,
                 v.reversed() ? r.begin().it_.getDomain().size() - r.begin().index_ :
                                r.end().index_)
    {
        // std::cout << "Restrictor for Variable "  << r.begin().view().v << " :
        // "
        // << r.lower() << " . . "<< r.upper() << " and indices " <<
        // r.begin().numElement() << " .. " << r.end().numElement() <<
        // std::endl;
        // std::cout << "Restrictor for View " << v.v << "*" << v.a << "+" <<
        // v.c <<
        // " : " << lower() << " . . " << upper() << " and indices " <<
        // begin().numElement() << " .. " << end().numElement() << std::endl;
        assert(r.begin().view().v == v.v);
        assert(r.begin().view().a == 1);
        assert(r.begin().view().c == 0);
    }

    Restrictor(const ViewIterator &l, const ViewIterator &u)
        : lower_{l}
        , upper_{u}
    {
    }

    ViewIterator begin() const { return lower_; }
    ViewIterator end() const { return upper_; }
    /// the number of elements, not max - min
    uint64 size() const
    {
        assert(upper_ - lower_ <= std::numeric_limits< uint32 >::max());
        return upper_ - lower_;
    }
    bool isEmpty() const { return lower_ == upper_; }

    /// compares two Restrictors of the very same domain only
    bool isSubsetEQOf(const Restrictor &r) const
    {
        assert(lower_.view() == r.lower_.view());
        return lower_ >= r.lower_ && upper_ <= r.upper_;
    }

    int64 lower() const { return *lower_; }
    int64 upper() const { return *(upper_ - 1); }

    friend std::ostream &operator<<(std::ostream &stream, const Restrictor &r);

private:
    ViewIterator lower_;
    ViewIterator upper_;
};

inline std::ostream &operator<<(std::ostream &stream, const Restrictor &r)
{
    for (auto i = r.lower_; i != r.upper_; ++i)
    {
        stream << *i << ", ";
    }

    return stream;
}

/*
class Restrictor
{
public:

    Restrictor(const Restrictor& r, View v) : lower_(r.begin(), v, 0),
upper_(r.end(), v, r.size()) { }
    /// construct a new LiteralRestrictor which is restricted to r
    Restrictor(const Restrictor& r, const Restrictor& lr) :
        lower_(r.begin(), lr.begin().v_, lr.begin().index_ +
(r.begin()-lr.begin().it_)),
        upper_(r.end(), lr.begin().v_, lr.end().index_ - (lr.end().it_-r.end()))
{ }

    class ViewIterator : public std::iterator<std::random_access_iterator_tag,
std::pair<int,Literal> >
    {
    public:
        friend LiteralRestrictor;
        ViewIterator() = default;
        ViewIterator(const ViewIterator& ) = default;
        ViewIterator& operator=(const ViewIterator&) = default;
        // please do only compare iterators of the same domain
        bool operator==(ViewIterator const &x) const { assert(v_ == x.v_);
assert((it_==x.it_) == (index_ == x.index_)); return index_ == x.index_; }
        bool operator!=(ViewIterator const &x) const { assert(v_ == x.v_);
assert((it_==x.it_) == (index_ == x.index_)); return index_ != x.index_; }

        ViewIterator& operator++() { ++it_; ++index_; return *this; }
        ViewIterator operator++(int) { auto temp(*this); ++it_; ++index_; return
temp; }

        ViewIterator& operator--()    { --it_; --index_; return *this; }
        ViewIterator operator--(int) { auto temp(*this); --it_; --index_; return
temp; }

        bool operator<(const ViewIterator& m) const { assert(v_ == m.v_);
assert((it_ < m.it_) == (index_ < m.index_) ); return index_ < m.index_; }
        bool operator>(const ViewIterator& m) const { assert(v_ == m.v_);
assert((it_ > m.it_) == (index_ > m.index_) ); return index_ > m.index_; }

        bool operator >=(const ViewIterator& m) const { assert(v_ == m.v_);
assert((it_ >= m.it_) == (index_ >= m.index_)); return !(*this < m); }
        bool operator <=(const ViewIterator& m) const { assert(v_ == m.v_);
assert((it_ <= m.it_) == (index_ <= m.index_)); return !(*this > m); }

        int64 operator-(const ViewIterator& m) const { assert(v_ == m.v_);
assert((it_ - m.it_) == ((int64)(index_) - (int64)(uint32)m.index_)); return
(int64)index_ - m.index_; } // this could overflow, but doesnt

        ViewIterator& operator+=(int64 x) { it_+=x; index_+=x; return *this; }
        ViewIterator& operator-=(int64 x) { it_-=x; index_-=x; return *this; }

        ViewIterator operator+(int64 x) const
        {
            ViewIterator tmp = *this;
            tmp+=x;
            return tmp;
        }

        ViewIterator operator-(int64 x) const
        {
            ViewIterator tmp = *this;
            tmp-=x;
            return tmp;
        }

        const ViewIterator& getIterator() const { return it_; }

        /// returns true if it is the first Literal of the order literals
        /// note: this can even be out of scope for the current
LiteralRestrictor
        bool isAtFirstLiteral() const { return index_==0; }

        View getView() const { return v_; }

        /// in case of a negative view, this index need to be converted by
storage
        /// do not use this function otherwise !
        unsigned int getIndex() const { return index_; }


        /// returns a value
        /// for accessing the literal -> VariableCreator.getLiteral
        int operator*() const { return *(it_); }
        int operator->() const { return *(it_); }
    private:
        ViewIterator(ViewIterator i, const View& v, std::size_t index) : it_(i),
v_(v), index_(index) { assert(v==i.view());}
        ViewIterator it_;
        View v_;
        unsigned int index_;
    };


    ViewIterator begin() const { return lower_; }
    ViewIterator end() const { return upper_; }
    uint32 size() const {
assert(upper_-lower_<=std::numeric_limits<uint32>::max()); return upper_-lower_;
}
    bool isEmpty() const { return lower_ == upper_; }

    int lower() const { return *lower_; }
    int upper() const { return *(upper_-1); }

    friend std::ostream& operator<< (std::ostream& stream, const Restrictor& r);

private:
    ViewIterator lower_;
    ViewIterator upper_;
};
*/
}
