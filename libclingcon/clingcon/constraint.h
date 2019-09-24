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
#include <clingcon/config.h>
#include <clingcon/solver.h>
#include <clingcon/storage.h>
#include <clingcon/types.h>
#include <clingcon/variable.h>

#include <algorithm>
#include <vector>

namespace clingcon
{

enum class Direction : int
{
    NONE = 0,
    FWD = 1,
    BACK = 2,
    EQ = 3,
};
inline Direction operator|(Direction lhs, Direction rhs)
{
    return static_cast< Direction >(static_cast< int >(lhs) | static_cast< int >(rhs));
}
inline bool operator&(Direction lhs, Direction rhs)
{
    return static_cast< bool >(static_cast< int >(lhs) & static_cast< int >(rhs));
}
inline Direction &operator|=(Direction &lhs, Direction rhs)
{
    lhs = static_cast< Direction >(static_cast< int >(lhs) | static_cast< int >(rhs));
    return lhs;
}
enum class TruthValue
{
    TRUE,
    FALSE,
    UNKNOWN
};
struct ReifiedLinearConstraint;

class LinearConstraint
{
public:
    friend ReifiedLinearConstraint;

    enum class Relation : short
    {
        LT,
        LE,
        GT,
        GE,
        EQ,
        NE
    };
    LinearConstraint(Relation r)
        : constant_(0)
        , r_(r)
        , flag_(false)
        , normalized_(false)
    {
    }
    // LinearConstraint(const LinearConstraint& o) : vars_(o.vars_),
    // constant_(o.constant_), r_(o.r_), flag_(o.flag_) {}
    // LinearConstraint(LinearConstraint&& o) : vars_(std::move(o.vars_)),
    // constant_(o.constant_), r_(o.r_), flag_(o.flag_) {}
    bool operator==(const LinearConstraint &r) const
    {
        return r_ == r.r_ && constant_ == r.constant_ && views_ == r.views_;
    }
    bool operator<(const LinearConstraint &r) const
    {
        return static_cast< int >(r_) < static_cast< int >(r.r_) && constant_ < r.constant_ &&
               views_ < r.views_;
    }

    Relation getRelation() const { return r_; }
    void setRelation(Relation r) { r_ = r; }
    std::vector< View > &getViews()
    {
        normalized_ = false;
        return views_;
    }
    const std::vector< View > &getViews() const { return views_; }
    const std::vector< View > &getConstViews() const { return views_; }
    void add(const View &v)
    {
        views_.emplace_back(v);
        normalized_ = false;
    }
    void addRhs(int constant)
    {
        constant_ += constant;
        normalized_ = false;
    }
    void times(int32 x);
    int getRhs() const { return constant_; }

    bool getFlag() const { return flag_; }
    void setFlag(bool b) { flag_ = b; }
    bool normalized() const { return normalized_; }

    friend std::ostream &operator<<(std::ostream &stream, const LinearConstraint &d);

    /// merge double variables, remove 0 coeffs
    /// performs and returns gcd
    int normalize();

    /// reverse the relation of the constraint
    void reverse();
    /// * -1 on both sides, inverts the relation is <,<=,>,=>
    void invert();

    /// sort the constraint according to |coefficient|, domainsize(var), biggest
    /// element first
    void sort(const VariableCreator &vc)
    {
        assert(normalized_);
        std::sort(views_.begin(), views_.end(), [&vc](const View &x, const View &y) {
            size_t a = vc.getDomainSize(x);
            size_t b = vc.getDomainSize(y);
            if (a != b) return a < b;
            return abs(x.a) > abs(y.a);
        });
    }

    /// pre: constraint is not empty
    /// product of size of all domains but last variable is less or equal x
    /// (x<0 is infinity)
    bool productOfDomainsExceptLastLEx(const VariableCreator &vc, int64 x) const;
    uint64 productOfDomainsExceptLast(const VariableCreator &vc) const;

    /// divide constraint by the gcd and return it
    /// pre: all views must have c==0
    int factorize();

private:
    std::vector< View > views_; /// should only contain views with b==1 and c==0
    int constant_;              // rhs
    Relation r_;
    bool flag_;
    bool normalized_;
};

inline std::ostream &operator<<(std::ostream &stream, const LinearConstraint &l)
{
    assert(l.normalized_);
    for (auto i = l.views_.begin(); i < l.views_.end() - 1; ++i)
    {
        stream << "v" << i->v << " * " << i->a << "\t+\t";
    }
    stream << "v" << (l.views_.end() - 1)->v << " * " << (l.views_.end() - 1)->a << "\t";
    switch (l.r_)
    {
    case LinearConstraint::Relation::EQ:
        stream << "== ";
        break;
    case LinearConstraint::Relation::NE:
        stream << "!= ";
        break;
    case LinearConstraint::Relation::LT:
        stream << "< ";
        break;
    case LinearConstraint::Relation::LE:
        stream << "<= ";
        break;
    case LinearConstraint::Relation::GT:
        stream << "> ";
        break;
    case LinearConstraint::Relation::GE:
        stream << ">= ";
        break;
    default:
        assert(true);
    }
    stream << l.constant_;

    return stream;
}

struct ReifiedLinearConstraint
{
    ReifiedLinearConstraint(LinearConstraint &&ll, const Literal &vv, Direction impl)
        : l(ll)
        , v(vv)
        , impl(impl)
    {
    }
    ReifiedLinearConstraint(const ReifiedLinearConstraint &) = default;

    /// sort without impl
    static bool compareless(const ReifiedLinearConstraint &l, const ReifiedLinearConstraint &r)
    {
        return std::tie(l.v, l.l) < std::tie(r.v, r.l); /*l.v<r.v && l.l<r.l && l.impl < r.impl;*/
    }
    /// ignore impl here
    static bool compareequal(const ReifiedLinearConstraint &l, const ReifiedLinearConstraint &r)
    {
        return l.v == r.v && l.l == r.l;
    }
    // bool operator==(const ReifiedLinearConstraint& r) const { return v==r.v
    // &&
    // l==r.l; }
    // bool operator<(const ReifiedLinearConstraint& r) const { return
    // /*std::tie(l,v) < std::tie(r.l,r.v);*/ v<r.v && l<r.l; }
    void sort(const VariableCreator &vc) { l.sort(vc); }
    LinearConstraint l;
    Literal v;
    Direction impl;
    /// reverse the literal
    void reverse() { l.reverse(); }
    /// this can change the literal as well as the constraint
    void normalize();
};

class ReifiedAllDistinct
{
public:
    /// TODO: sort variables, detect subset relations, reuse intermediate
    /// variables etc...
    ReifiedAllDistinct(std::vector< View > &&views, const Literal &l, Direction impl)
        : views_(std::move(views))
        , v_(l)
        , impl_(impl)
    {
        std::sort(views_.begin(), views_.end());
        views_.erase(std::unique(views_.begin(), views_.end()), views_.end());
    }
    Direction getDirection() const { return impl_; }
    void add(const Variable &v) { views_.emplace_back(v); }
    const std::vector< View > &getViews() const { return views_; }
    std::vector< View > &getViews() { return views_; }
    void times(int32 x) /// multiply all views by x
    {
        for (auto &i : views_) i *= x;
    }

    Literal getLiteral() const { return v_; }
    void setLiteral(const Literal &l) { v_ = l; }

private:
    std::vector< View > views_;
    Literal v_;
    Direction impl_;
};

/// if the literal l is true, the view v has one value of the domain d
/// if false, it is unequal to the values in d
///
class ReifiedDomainConstraint
{
public:
    ReifiedDomainConstraint(View v, Domain &&d, const Literal &l, Direction impl)
        : v_(v)
        , d_(std::move(d))
        , l_(l)
        , impl_(impl)
    {
    }
    ReifiedDomainConstraint(const ReifiedDomainConstraint &c) = default;
    ReifiedDomainConstraint(ReifiedDomainConstraint &&m) = default;
    ReifiedDomainConstraint &operator=(ReifiedDomainConstraint &&m) = default;
    ReifiedDomainConstraint &operator=(const ReifiedDomainConstraint &a) = default;
    Direction getDirection() const { return impl_; }
    View getView() const { return v_; }
    View &getView() { return v_; }
    Literal getLiteral() const { return l_; }
    void setLiteral(const Literal &l) { l_ = l; }
    const Domain &getDomain() const { return d_; }
    Domain &getDomain() { return d_; }

private:
    View v_;
    Domain d_;
    Literal l_;
    Direction impl_;
};
} // namespace clingcon
