// {{{ MIT License
//
// Copyright 2020 Roland Kaminski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
// }}}

#ifndef CLINGCON_CONSTRAINTS_H
#define CLINGCON_CONSTRAINTS_H

#include <clingcon/solver.hh>

#include <array>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4200)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

//! @file clingcon/constraints.hh
//! Module implementing constraints.
//!
//! @author Roland Kaminski

namespace Clingcon {

//! Class to capture sum constraints of form `a_0*x_0 + ... + a_n * x_n <= rhs`.
class SumConstraint final : public AbstractConstraint {
public:
    SumConstraint() = delete;
    SumConstraint(SumConstraint const &) = delete;
    SumConstraint(SumConstraint &&) = delete;
    SumConstraint &operator=(SumConstraint const &) = delete;
    SumConstraint &operator=(SumConstraint &&) = delete;
    ~SumConstraint() override = default;

    //! Create a new sum constraint.
    [[nodiscard]] static std::unique_ptr<SumConstraint> create(lit_t lit, val_t rhs, CoVarVec const &elems, bool sort) {
        auto size = sizeof(SumConstraint) + elems.size() * sizeof(std::pair<val_t, var_t>);
        return std::unique_ptr<SumConstraint>{new (operator new(size)) SumConstraint(lit, rhs, elems, sort)};
    }

    //! Create thread specific state for the constraint.
    [[nodiscard]] UniqueConstraintState create_state() override;

    //! Get the literal associated with the constraint.
    [[nodiscard]] lit_t literal() const override {
        return lit_;
    }

    //! Get the rhs of the consraint.
    [[nodiscard]] val_t rhs() const {
        return rhs_;
    }

    //! Get the number of elements in the constraint.
    [[nodiscard]] size_t size() const {
        return size_;
    }

    //! Access the i-th element.
    [[nodiscard]] std::pair<val_t, var_t> operator[](size_t i) const {
        return elements_[i];
    }

    //! Pointer to the first element of the constraint.
    [[nodiscard]] std::pair<val_t, var_t> const *begin() const {
        return elements_;
    }

    //! Pointer after the last element of the constraint.
    [[nodiscard]] std::pair<val_t, var_t> const *end() const {
        return elements_ + size_; // NOLINT
    }

private:
    SumConstraint(lit_t lit, val_t rhs, CoVarVec const &elems, bool sort)
    : lit_{lit}
    , rhs_{rhs}
    , size_{elems.size()} {
        std::copy(elems.begin(), elems.end(), elements_);
        if (sort) {
            std::sort(elements_, elements_ + size_, [](auto a, auto b) { return std::abs(a.first) > std::abs(b.first); } ); // NOLINT
        }
    }

    //! Solver literal associated with the constraint.
    lit_t lit_;
    //! Integer bound of the constraint.
    val_t rhs_;
    //! Number of elements in the constraint.
    size_t size_;
    //! List of integer/string pairs representing coefficient and variable.
    std::pair<val_t, var_t> elements_[]; // NOLINT
};

//! Class to capture nonlinear constraints of form `ab*v_a*v_b + c*v_c <= rhs`
class NonlinearConstraint final : public AbstractConstraint {
public:
    NonlinearConstraint(lit_t lit, val_t co_a, var_t var_x, var_t var_y, val_t co_b, var_t var_z, val_t rhs)
    : lit_{lit}
    , rhs_{rhs}
    , co_a_{co_a}
    , var_x_{var_x}
    , var_y_{var_y}
    , co_b_{co_b}
    , var_z_{var_z} { }
    NonlinearConstraint() = delete;
    NonlinearConstraint(NonlinearConstraint const &) = delete;
    NonlinearConstraint(NonlinearConstraint &&) = delete;
    NonlinearConstraint &operator=(NonlinearConstraint const &) = delete;
    NonlinearConstraint &operator=(NonlinearConstraint &&) = delete;
    ~NonlinearConstraint() override = default;

    //! Create thread specific state for the constraint.
    [[nodiscard]] UniqueConstraintState create_state() override;

    //! Get the literal associated with the constraint.
    [[nodiscard]] lit_t literal() const override {
        return lit_;
    }
    //! Get the coefficient of the nonlinear term.
    [[nodiscard]] val_t co_a() const {
        return co_a_;
    }
    //! Get the first variable of the nonlinear term.
    [[nodiscard]] var_t var_x() const {
        return var_x_;
    }
    //! Get the second variable of the nonlinear term.
    [[nodiscard]] var_t var_y() const {
        return var_y_;
    }
    //! Check if the costraint has a linear term.
    [[nodiscard]] bool has_co_c() const {
        return co_b_ != 0;
    }
    //! Get the coefficient of the linear term.
    [[nodiscard]] val_t co_b() const {
        return co_b_;
    }
    //! Get the variable of the linear term.
    [[nodiscard]] var_t var_z() const {
        return var_z_;
    }
    //! Get the rhs of the consraint.
    [[nodiscard]] val_t rhs() const {
        return rhs_;
    }

private:
    //! Solver literal associated with the constraint.
    lit_t lit_;
    //! Integer bound of the constraint.
    val_t rhs_;
    //! Nonzero coefficient of the nonlinear term.
    val_t co_a_;
    //! First variable of the nonlinear term.
    var_t var_x_;
    //! Second variable of the nonlinear term.
    var_t var_y_;
    //! Coefficient of the linear term.
    val_t co_b_;
    //! Variable of the linear term.
    var_t var_z_;
};

//! Class to capture minimize constraints of form `a_0*x_0 + ... + a_n * x_n + adjust`.
class MinimizeConstraint final : public AbstractConstraint {
public:
    MinimizeConstraint() = delete;
    MinimizeConstraint(MinimizeConstraint &) = delete;
    MinimizeConstraint(MinimizeConstraint &&) = delete;
    MinimizeConstraint &operator=(MinimizeConstraint const &) = delete;
    MinimizeConstraint &operator=(MinimizeConstraint &&) = delete;
    ~MinimizeConstraint() override = default;

    //! Create a new sum constraint.
    [[nodiscard]] static std::unique_ptr<MinimizeConstraint> create(val_t adjust, CoVarVec const &elems, bool sort) {
        auto size = sizeof(MinimizeConstraint) + elems.size() * sizeof(std::pair<val_t, var_t>);
        return std::unique_ptr<MinimizeConstraint>{new (operator new(size)) MinimizeConstraint(adjust, elems, sort)};
    }

    //! Create thread specific state for the constraint.
    [[nodiscard]] UniqueConstraintState create_state() override;

    //! Get the literal associated with the constraint.
    [[nodiscard]] lit_t literal() const override {
        return TRUE_LIT;
    }

    //! Get the adjustment of the consraint.
    [[nodiscard]] val_t adjust() const {
        return adjust_;
    }

    //! Get the number of elements in the constraint.
    [[nodiscard]] size_t size() const {
        return size_;
    }

    //! Access the i-th element.
    [[nodiscard]] std::pair<val_t, var_t> operator[](size_t i) const {
        return elements_[i];
    }

    //! Pointer to the first element of the constraint.
    [[nodiscard]] std::pair<val_t, var_t> const *begin() const {
        return elements_;
    }

    //! Pointer after the last element of the constraint.
    [[nodiscard]] std::pair<val_t, var_t> const *end() const {
        return elements_ + size_; // NOLINT
    }

private:
    MinimizeConstraint(val_t adjust, CoVarVec const &elems, bool sort)
    : adjust_{adjust}
    , size_{static_cast<uint32_t>(elems.size())} {
        std::copy(elems.begin(), elems.end(), elements_);
        if (sort) {
            std::sort(elements_, elements_ + size_, [](auto a, auto b) { return std::abs(a.first) > std::abs(b.first); } ); // NOLINT
        }
    }

    //! Integer adjustment of the constraint.
    val_t adjust_;
    //! Number of elements in the constraint.
    uint32_t size_;
    //! List of integer/string pairs representing coefficient and variable.
    std::pair<val_t, var_t> elements_[]; // NOLINT
};

class DistinctElement {
public:
#ifdef _MSC_VER
    DistinctElement()
    : fixed_{0}
    , size_{0}
    , elements_{nullptr} {
    }
#endif
    DistinctElement(val_t fixed, size_t size, co_var_t *elements, bool sort);

    //! Get the fixed part of the term.
    [[nodiscard]] val_t fixed() const {
        return fixed_;
    }

    //! Check if the element is constant.
    [[nodiscard]] bool empty() const {
        return size_ == 0;
    }

    //! Get the number of elements in the constraint.
    [[nodiscard]] size_t size() const {
        return size_;
    }

    //! Access the i-th element.
    [[nodiscard]] co_var_t const &operator[](size_t i) const {
        return elements_[i]; // NOLINT
    }

    //! Pointer to the first element of the constraint.
    [[nodiscard]] co_var_t *begin() const {
        return elements_;
    }

    //! Pointer after the last element of the constraint.
    [[nodiscard]] co_var_t *end() const {
        return elements_ + size_; // NOLINT
    }

private:
    val_t fixed_;
    uint32_t size_;
    co_var_t *elements_;
};

//! Class to capture distinct constraints.
class DistinctConstraint final : public AbstractConstraint {
public:
    using Elements = std::vector<std::pair<CoVarVec, val_t>>;

    //! Create a new distinct constraint.
    [[nodiscard]] static std::unique_ptr<DistinctConstraint> create(lit_t lit, Elements const &elements, bool sort);

    DistinctConstraint() = delete;
    DistinctConstraint(DistinctConstraint &) = delete;
    DistinctConstraint(DistinctConstraint &&) = delete;
    DistinctConstraint &operator=(DistinctConstraint const &) = delete;
    DistinctConstraint &operator=(DistinctConstraint &&) = delete;
    ~DistinctConstraint() override = default;

    //! Create thread specific state for the constraint.
    [[nodiscard]] UniqueConstraintState create_state() override;

    //! Get the literal associated with the constraint.
    [[nodiscard]] lit_t literal() const override {
        return lit_;
    }

    //! Get the number of elements in the constraint.
    [[nodiscard]] size_t size() const {
        return size_;
    }

    //! Access the i-th element.
    [[nodiscard]] DistinctElement const &operator[](size_t i) const {
        return elements_[i]; // NOLINT
    }

    //! Pointer to the first element of the constraint.
    [[nodiscard]] DistinctElement const *begin() const {
        return elements_;
    }

    //! Pointer after the last element of the constraint.
    [[nodiscard]] DistinctElement const *end() const {
        return elements_ + size_; // NOLINT
    }

private:
    DistinctConstraint(lit_t lit, Elements const &elements, bool sort);

    //! Solver literal associated with the constraint.
    lit_t lit_;
    //! The elements of the distinct constraint.
    uint32_t size_;
    //! List of integer/string pairs representing coefficient and variable.
    DistinctElement elements_[]; // NOLINT
};

//! Class to capture disjoint constraints.
class DisjointConstraint final : public AbstractConstraint {
public:
    //! Create a new distinct constraint.
    [[nodiscard]] static std::unique_ptr<DisjointConstraint> create(lit_t lit, CoVarVec const &elements);

    DisjointConstraint() = delete;
    DisjointConstraint(DisjointConstraint &) = delete;
    DisjointConstraint(DisjointConstraint &&) = delete;
    DisjointConstraint &operator=(DisjointConstraint const &) = delete;
    DisjointConstraint &operator=(DisjointConstraint &&) = delete;
    ~DisjointConstraint() override = default;

    //! Create thread specific state for the constraint.
    [[nodiscard]] UniqueConstraintState create_state() override;

    //! Get the literal associated with the constraint.
    [[nodiscard]] lit_t literal() const override {
        return lit_;
    }

    //! Get the number of elements in the constraint.
    [[nodiscard]] size_t size() const {
        return size_;
    }

    //! Access the i-th element.
    [[nodiscard]] co_var_t const &operator[](size_t i) const {
        return elements_[i]; // NOLINT
    }

    //! Pointer to the first element of the constraint.
    [[nodiscard]] co_var_t const *begin() const {
        return elements_;
    }

    //! Pointer after the last element of the constraint.
    [[nodiscard]] co_var_t const *end() const {
        return elements_ + size_; // NOLINT
    }

private:
    DisjointConstraint(lit_t lit, CoVarVec const &elements);

    //! Solver literal associated with the constraint.
    lit_t lit_;
    //! The elements of the distinct constraint.
    uint32_t size_;
    //! List of integer/string pairs representing coefficient and variable.
    co_var_t elements_[]; // NOLINT
};

} // namespace Clingcon

#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#endif // CLINGCON_CONSTRAINTS_H
