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
    [[nodiscard]] static std::unique_ptr<SumConstraint> create(lit_t lit, val_t rhs, CoVarVec &elems) {
        auto size = sizeof(SumConstraint) + elems.size() * sizeof(std::pair<val_t, var_t>);
        return std::unique_ptr<SumConstraint>{new (operator new(size)) SumConstraint(lit, rhs, elems)};
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
    SumConstraint(lit_t lit, val_t rhs, CoVarVec &elems)
    : lit_{lit}
    , rhs_{rhs}
    , size_{elems.size()} {
        std::copy(elems.begin(), elems.end(), elements_);
    }

    //! Solver literal associated with the constraint.
    lit_t lit_;
    //! Integer bound of the constraint.
    val_t rhs_;
    //! Number of elements in the constraint.
    size_t size_;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    //! List of integer/string pairs representing coefficient and variable.
    std::pair<val_t, var_t> elements_[]; // NOLINT
#pragma GCC diagnostic pop
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
    [[nodiscard]] static std::unique_ptr<MinimizeConstraint> create(lit_t lit, val_t adjust, CoVarVec &elems) {
        auto size = sizeof(MinimizeConstraint) + elems.size() * sizeof(std::pair<val_t, var_t>);
        return std::unique_ptr<MinimizeConstraint>{new (operator new(size)) MinimizeConstraint(lit, adjust, elems)};
    }

    //! Create thread specific state for the constraint.
    [[nodiscard]] UniqueConstraintState create_state() override;

    //! Get the literal associated with the constraint.
    [[nodiscard]] lit_t literal() const override {
        return lit_;
    }

    //! Get the adjustment of the consraint.
    [[nodiscard]] val_t rhs() const {
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
    MinimizeConstraint(lit_t lit, val_t adjust, CoVarVec &elems)
    : lit_{lit}
    , adjust_{adjust}
    , size_{elems.size()} {
        std::copy(elems.begin(), elems.end(), elements_);
    }

    //! Solver literal associated with the constraint.
    lit_t lit_;
    //! Integer bound of the constraint.
    val_t adjust_;
    //! Number of elements in the constraint.
    size_t size_;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    //! List of integer/string pairs representing coefficient and variable.
    std::pair<val_t, var_t> elements_[]; // NOLINT
#pragma GCC diagnostic pop
};

//! Class to capture distinct constraints.
class DistinctConstraint final : public AbstractConstraint {
public:
    using Term = std::pair<CoVarVec,val_t>;
    using Elements = std::vector<Term>;
    using Iterator = Elements::const_iterator;

    DistinctConstraint(lit_t lit, Elements elements)
    : lit_{lit}
    , elements_{std::move(elements)} {
    }

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
        return elements_.size();
    }

    //! Access the i-th element.
    [[nodiscard]] Term const &operator[](size_t i) const {
        return elements_[i];
    }

    //! Pointer to the first element of the constraint.
    [[nodiscard]] Iterator begin() const {
        return elements_.begin();
    }

    //! Pointer after the last element of the constraint.
    [[nodiscard]] Iterator end() const {
        return elements_.end();
    }

private:
    //! Solver literal associated with the constraint.
    lit_t lit_;
    //! The elements of the distinct constraint.
    Elements elements_;
};

} // namespace

#endif // CLINGCON_CONSTRAINTS_H
