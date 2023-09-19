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
#pragma warning(disable : 4200)
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
    auto operator=(SumConstraint const &) -> SumConstraint & = delete;
    auto operator=(SumConstraint &&) -> SumConstraint & = delete;
    ~SumConstraint() override = default;

    //! Create a new sum constraint.
    [[nodiscard]] static auto create(lit_t lit, val_t rhs, CoVarVec const &elems, bool sort)
        -> std::unique_ptr<SumConstraint> {
        auto size = sizeof(SumConstraint) + elems.size() * sizeof(std::pair<val_t, var_t>);
        return std::unique_ptr<SumConstraint>{new (operator new(size)) SumConstraint(lit, rhs, elems, sort)};
    }

    //! Create thread specific state for the constraint.
    [[nodiscard]] auto create_state() -> UniqueConstraintState override;

    //! Get the literal associated with the constraint.
    [[nodiscard]] auto literal() const -> lit_t override { return lit_; }

    //! Get the rhs of the consraint.
    [[nodiscard]] auto rhs() const -> val_t { return rhs_; }

    //! Get the number of elements in the constraint.
    [[nodiscard]] auto size() const -> size_t { return size_; }

    //! Access the i-th element.
    [[nodiscard]] auto operator[](size_t i) const -> std::pair<val_t, var_t> { return elements_[i]; }

    //! Pointer to the first element of the constraint.
    [[nodiscard]] auto begin() const -> std::pair<val_t, var_t> const * { return elements_; }

    //! Pointer after the last element of the constraint.
    [[nodiscard]] auto end() const -> std::pair<val_t, var_t> const * {
        return elements_ + size_; // NOLINT
    }

  private:
    SumConstraint(lit_t lit, val_t rhs, CoVarVec const &elems, bool sort) : lit_{lit}, rhs_{rhs}, size_{elems.size()} {
        std::copy(elems.begin(), elems.end(), elements_);
        if (sort) {
            std::sort(elements_, elements_ + size_,
                      [](auto a, auto b) { return std::abs(a.first) > std::abs(b.first); }); // NOLINT
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
        : lit_{lit}, rhs_{rhs}, co_a_{co_a}, var_x_{var_x}, var_y_{var_y}, co_b_{co_b}, var_z_{var_z} {}
    NonlinearConstraint() = delete;
    NonlinearConstraint(NonlinearConstraint const &) = delete;
    NonlinearConstraint(NonlinearConstraint &&) = delete;
    auto operator=(NonlinearConstraint const &) -> NonlinearConstraint & = delete;
    auto operator=(NonlinearConstraint &&) -> NonlinearConstraint & = delete;
    ~NonlinearConstraint() override = default;

    //! Create thread specific state for the constraint.
    [[nodiscard]] auto create_state() -> UniqueConstraintState override;

    //! Get the literal associated with the constraint.
    [[nodiscard]] auto literal() const -> lit_t override { return lit_; }
    //! Get the coefficient of the nonlinear term.
    [[nodiscard]] auto co_a() const -> val_t { return co_a_; }
    //! Get the first variable of the nonlinear term.
    [[nodiscard]] auto var_x() const -> var_t { return var_x_; }
    //! Get the second variable of the nonlinear term.
    [[nodiscard]] auto var_y() const -> var_t { return var_y_; }
    //! Check if the costraint has a linear term.
    [[nodiscard]] auto has_co_c() const -> bool { return co_b_ != 0; }
    //! Get the coefficient of the linear term.
    [[nodiscard]] auto co_b() const -> val_t { return co_b_; }
    //! Get the variable of the linear term.
    [[nodiscard]] auto var_z() const -> var_t { return var_z_; }
    //! Get the rhs of the consraint.
    [[nodiscard]] auto rhs() const -> val_t { return rhs_; }

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
    auto operator=(MinimizeConstraint const &) -> MinimizeConstraint & = delete;
    auto operator=(MinimizeConstraint &&) -> MinimizeConstraint & = delete;
    ~MinimizeConstraint() override = default;

    //! Create a new sum constraint.
    [[nodiscard]] static auto create(val_t adjust, CoVarVec const &elems, bool sort)
        -> std::unique_ptr<MinimizeConstraint> {
        auto size = sizeof(MinimizeConstraint) + elems.size() * sizeof(std::pair<val_t, var_t>);
        return std::unique_ptr<MinimizeConstraint>{new (operator new(size)) MinimizeConstraint(adjust, elems, sort)};
    }

    //! Create thread specific state for the constraint.
    [[nodiscard]] auto create_state() -> UniqueConstraintState override;

    //! Get the literal associated with the constraint.
    [[nodiscard]] auto literal() const -> lit_t override { return TRUE_LIT; }

    //! Get the adjustment of the consraint.
    [[nodiscard]] auto adjust() const -> val_t { return adjust_; }

    //! Get the number of elements in the constraint.
    [[nodiscard]] auto size() const -> size_t { return size_; }

    //! Access the i-th element.
    [[nodiscard]] auto operator[](size_t i) const -> std::pair<val_t, var_t> { return elements_[i]; }

    //! Pointer to the first element of the constraint.
    [[nodiscard]] auto begin() const -> std::pair<val_t, var_t> const * { return elements_; }

    //! Pointer after the last element of the constraint.
    [[nodiscard]] auto end() const -> std::pair<val_t, var_t> const * {
        return elements_ + size_; // NOLINT
    }

  private:
    MinimizeConstraint(val_t adjust, CoVarVec const &elems, bool sort)
        : adjust_{adjust}, size_{static_cast<uint32_t>(elems.size())} {
        std::copy(elems.begin(), elems.end(), elements_);
        if (sort) {
            std::sort(elements_, elements_ + size_,
                      [](auto a, auto b) { return std::abs(a.first) > std::abs(b.first); }); // NOLINT
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
    DistinctElement() : fixed_{0}, size_{0}, elements_{nullptr} {}
#endif
    DistinctElement(val_t fixed, size_t size, co_var_t *elements, bool sort);

    //! Get the fixed part of the term.
    [[nodiscard]] auto fixed() const -> val_t { return fixed_; }

    //! Check if the element is constant.
    [[nodiscard]] auto empty() const -> bool { return size_ == 0; }

    //! Get the number of elements in the constraint.
    [[nodiscard]] auto size() const -> size_t { return size_; }

    //! Access the i-th element.
    [[nodiscard]] auto operator[](size_t i) const -> co_var_t const & {
        return elements_[i]; // NOLINT
    }

    //! Pointer to the first element of the constraint.
    [[nodiscard]] auto begin() const -> co_var_t * { return elements_; }

    //! Pointer after the last element of the constraint.
    [[nodiscard]] auto end() const -> co_var_t * {
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
    [[nodiscard]] static auto create(lit_t lit, Elements const &elements, bool sort)
        -> std::unique_ptr<DistinctConstraint>;

    DistinctConstraint() = delete;
    DistinctConstraint(DistinctConstraint &) = delete;
    DistinctConstraint(DistinctConstraint &&) = delete;
    auto operator=(DistinctConstraint const &) -> DistinctConstraint & = delete;
    auto operator=(DistinctConstraint &&) -> DistinctConstraint & = delete;
    ~DistinctConstraint() override = default;

    //! Create thread specific state for the constraint.
    [[nodiscard]] auto create_state() -> UniqueConstraintState override;

    //! Get the literal associated with the constraint.
    [[nodiscard]] auto literal() const -> lit_t override { return lit_; }

    //! Get the number of elements in the constraint.
    [[nodiscard]] auto size() const -> size_t { return size_; }

    //! Access the i-th element.
    [[nodiscard]] auto operator[](size_t i) const -> DistinctElement const & {
        return elements_[i]; // NOLINT
    }

    //! Pointer to the first element of the constraint.
    [[nodiscard]] auto begin() const -> DistinctElement const * { return elements_; }

    //! Pointer after the last element of the constraint.
    [[nodiscard]] auto end() const -> DistinctElement const * {
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
    [[nodiscard]] static auto create(lit_t lit, CoVarVec const &elements) -> std::unique_ptr<DisjointConstraint>;

    DisjointConstraint() = delete;
    DisjointConstraint(DisjointConstraint &) = delete;
    DisjointConstraint(DisjointConstraint &&) = delete;
    auto operator=(DisjointConstraint const &) -> DisjointConstraint & = delete;
    auto operator=(DisjointConstraint &&) -> DisjointConstraint & = delete;
    ~DisjointConstraint() override = default;

    //! Create thread specific state for the constraint.
    [[nodiscard]] auto create_state() -> UniqueConstraintState override;

    //! Get the literal associated with the constraint.
    [[nodiscard]] auto literal() const -> lit_t override { return lit_; }

    //! Get the number of elements in the constraint.
    [[nodiscard]] auto size() const -> size_t { return size_; }

    //! Access the i-th element.
    [[nodiscard]] auto operator[](size_t i) const -> co_var_t const & {
        return elements_[i]; // NOLINT
    }

    //! Pointer to the first element of the constraint.
    [[nodiscard]] auto begin() const -> co_var_t const * { return elements_; }

    //! Pointer after the last element of the constraint.
    [[nodiscard]] auto end() const -> co_var_t const * {
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
