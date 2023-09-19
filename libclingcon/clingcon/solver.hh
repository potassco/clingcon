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

#ifndef CLINGCON_SOLVER_H
#define CLINGCON_SOLVER_H

#include <clingcon/base.hh>
#include <clingcon/util.hh>
#include <map>
#include <unordered_map>

//! @file clingcon/solver.hh
//! This module implements a CSP solver for thread-specific propagation and
//! defines interfaces for constraints.
//!
//! @author Roland Kaminski

namespace Clingcon {

class Solver;
class AbstractConstraint;
class AbstractConstraintState;
using UniqueConstraint = std::unique_ptr<AbstractConstraint>;
using ConstraintVec = std::vector<UniqueConstraint>;
using UniqueConstraintState = std::unique_ptr<AbstractConstraintState>;

constexpr val_t MOGRIFY_FACTOR = 10;

//! Base class of all constraints.
class AbstractConstraint {
  public:
    AbstractConstraint() = default;
    AbstractConstraint(AbstractConstraint const &) = delete;
    AbstractConstraint(AbstractConstraint &&) = delete;
    auto operator=(AbstractConstraint const &) -> AbstractConstraint & = delete;
    auto operator=(AbstractConstraint &&) -> AbstractConstraint & = delete;
    virtual ~AbstractConstraint() = default;

    //! Create thread specific state for the constraint.
    [[nodiscard]] virtual auto create_state() -> UniqueConstraintState = 0;

    //! Get the literal associated with the constraint.
    [[nodiscard]] virtual auto literal() const -> lit_t = 0;
};

//! Abstract class to capture the state of constraints.
class AbstractConstraintState {
  public:
    AbstractConstraintState() = default;
    AbstractConstraintState(AbstractConstraintState const &) = delete;
    AbstractConstraintState(AbstractConstraintState &&) = delete;
    auto operator=(AbstractConstraintState const &) -> AbstractConstraintState & = delete;
    auto operator=(AbstractConstraintState &&) -> AbstractConstraintState & = delete;
    virtual ~AbstractConstraintState() = default;

    //! Get the associated constraint.
    virtual auto constraint() -> AbstractConstraint & = 0;

    //! @name Initialization Functions
    //! @{

    //! Attach the constraint to a solver.
    virtual void attach(Solver &solver) = 0;
    //! Detach the constraint from a solver.
    virtual void detach(Solver &solver) = 0;
    //! Translate a constraint to simpler constraints.
    [[nodiscard]] virtual auto translate(Config const &config, Solver &solver, InitClauseCreator &cc,
                                         ConstraintVec &added) -> std::pair<bool, bool> = 0;
    //! Copy the constraint state (for another solver)
    [[nodiscard]] virtual auto copy() const -> UniqueConstraintState = 0;

    //! @}

    //! @name Functions for Constraint Progation
    //! @{

    //! Inform the solver about updated bounds of a variable.
    //!
    //! Value i depends on the value passed when registering the watch and diff
    //! is the change to the bound of the watched variable.
    [[nodiscard]] virtual auto update(val_t i, val_t diff) -> bool = 0;
    //! Similar to update but when the bound of a variable is backtracked.
    virtual void undo(val_t i, val_t diff) = 0;
    //! Prepagates the constraint.
    [[nodiscard]] virtual auto propagate(Solver &solver, AbstractClauseCreator &cc, bool check_state) -> bool = 0;
    //! Check if the solver meets the state invariants.
    virtual void check_full(Solver &solver) = 0;

    //! @}

    //! @name Functions to Manage Todo Sets
    //! @{

    //! Mark the constraint state as todo item.
    virtual auto mark_todo(bool todo) -> bool = 0;
    //! Returns true if the constraint is marked as todo item.
    [[nodiscard]] virtual auto marked_todo() const -> bool = 0;

    //! @}

    //! @name Functions to Manage Sets of Inactive Constraint States (Template)
    //! @{

    //! Returns true if the constraint is marked inactive.
    [[nodiscard]] auto marked_inactive() const -> bool { return inactive_level() > 0; }
    //! Returns true if the constraint is removable.
    [[nodiscard]] virtual auto removable() -> bool = 0;
    //! Mark a constraint inactive on the given level.
    void mark_inactive(level_t level) {
        assert(!marked_inactive());
        inactive_level(level + 1);
    }
    //! Mark a constraint active.
    void mark_active() { inactive_level(0); }
    //! A constraint is removable if it has been marked inactive on a lower
    //! level.
    [[nodiscard]] auto removable(level_t level) -> bool { return marked_inactive() && inactive_level() <= level; }

    //! @}

  protected:
    //! @name Functions Manage Sets of Inactive Constraint States (Virtual)
    //! @{

    //! Get the level on which the constraint became inactive.
    [[nodiscard]] virtual auto inactive_level() const -> level_t = 0;
    //! Set the level on which the constraint became inactive.
    virtual void inactive_level(level_t level) = 0;

    //! @}
};

//! Class to facilitate handling order literals associated with an integer
//! variable.
//!
//! The class maintains a stack of lower and upper bounds, which initially
//! contain the smallest and largest allowed integer.
class VarState {
    using OrderMap = std::map<val_t, lit_t>;                     //!< Map to store order literals.
    using OrderVec = std::vector<lit_t>;                         //!< Vetor to store order literals.
    using ReverseIteratorMap = OrderMap::const_reverse_iterator; //!< Reverse iterator over order literals in map.
    using IteratorVec = OrderVec::const_iterator;                //!< Iterator over order literals in vector.
    using ReverseIteratorVec = OrderVec::const_reverse_iterator; //!< Reverse iterator over order literals in vector.
    using BoundStack = std::vector<std::pair<level_t, val_t>>;   //!< Container for stacks of lower/upper bounds.
    template <typename F, typename It>
    using RetType =
        std::invoke_result_t<F, It, It, std::function<lit_t(It)>, std::function<val_t(It)>, std::function<void(It)>>;
    static constexpr val_t unused = std::numeric_limits<val_t>::min();

  public:
    using OrderLiteral = std::pair<lit_t, val_t>;
    using OrderLiteralOpt = std::optional<OrderLiteral>;

    //! Create an initial state for the given variable.
    //!
    //! Initially, the state should have  a lower bound of `Config::min_int`
    //! and an upper bound of `Config::max_int` and is associated with no
    //! variables.
    VarState() = delete;
    VarState(var_t var, val_t lower_bound, val_t upper_bound)
        : var_{var}, lower_bound_{lower_bound}, upper_bound_{upper_bound} {
        new (&litmap_) OrderMap();
    }

    VarState(VarState const &x)
        : var_{x.var_}, lower_bound_{x.lower_bound_}, upper_bound_{x.upper_bound_}, offset_{x.offset_},
          lower_bound_stack_{x.lower_bound_stack_}, upper_bound_stack_{x.upper_bound_stack_} {
        if (offset_ == unused) {
            new (&litmap_) OrderMap(x.litmap_);
        } else {
            new (&litvec_) OrderVec(x.litvec_);
        }
    }

    VarState(VarState &&x) noexcept
        : var_{x.var_}, lower_bound_{x.lower_bound_}, upper_bound_{x.upper_bound_}, offset_{x.offset_},
          lower_bound_stack_{std::move(x.lower_bound_stack_)}, upper_bound_stack_{std::move(x.upper_bound_stack_)} {
        if (offset_ == unused) {
            new (&litmap_) OrderMap(std::move(x.litmap_));
        } else {
            new (&litvec_) OrderVec(std::move(x.litvec_));
        }
    }

    auto operator=(VarState const &x) -> VarState & {
        var_ = x.var_;
        lower_bound_ = x.lower_bound_;
        upper_bound_ = x.upper_bound_;
        lower_bound_stack_ = x.lower_bound_stack_;
        upper_bound_stack_ = x.upper_bound_stack_;
        if (x.offset_ == unused) {
            if (offset_ != unused) {
                litvec_.~OrderVec();
                new (&litmap_) OrderMap();
            }
            offset_ = unused;
            litmap_ = x.litmap_;
        } else {
            if (offset_ == unused) {
                litmap_.~OrderMap();
                new (&litvec_) OrderVec(x.litvec_);
            }
            offset_ = x.offset_;
            litvec_ = x.litvec_;
        }
        return *this;
    }

    auto operator=(VarState &&x) noexcept -> VarState & {
        var_ = x.var_;
        lower_bound_ = x.lower_bound_;
        upper_bound_ = x.upper_bound_;
        lower_bound_stack_ = std::move(x.lower_bound_stack_);
        upper_bound_stack_ = std::move(x.upper_bound_stack_);
        if (x.offset_ == unused) {
            if (offset_ == unused) {
                litmap_ = std::move(x.litmap_);
            } else {
                litvec_.~OrderVec();
                new (&litmap_) OrderMap(std::move(x.litmap_));
            }
        } else {
            if (offset_ == unused) {
                litmap_.~OrderMap();
                new (&litvec_) OrderVec(std::move(x.litvec_));
            } else {
                litvec_ = std::move(x.litvec_);
            }
        }
        offset_ = x.offset_;
        return *this;
    }

    ~VarState() {
        if (offset_ == unused) {
            litmap_.~OrderMap();
        } else {
            litvec_.~OrderVec();
        }
    }

    //! Remove all literals associated with this state.
    void reset(val_t min_int, val_t max_int) {
        lower_bound_ = min_int;
        upper_bound_ = max_int;
        lower_bound_stack_.clear();
        upper_bound_stack_.clear();
        litmap_.clear();
        litvec_.clear();
        offset_ = unused;
    }

    //! @name Functions for Lower Bounds
    //! @{

    //! Get current lower bound.
    [[nodiscard]] auto lower_bound() const -> val_t { return lower_bound_; }
    //! Set new lower bound.
    void lower_bound(val_t lower_bound) {
        assert(lower_bound >= lower_bound_);
        lower_bound_ = lower_bound;
    }
    //! Push the current lower bound on the stack.
    void push_lower(level_t level) { lower_bound_stack_.emplace_back(level, lower_bound_); }
    //! Check if the given level has already been pushed on the stack.
    [[nodiscard]] auto pushed_lower(level_t level) const -> bool {
        return !lower_bound_stack_.empty() && lower_bound_stack_.back().first == level;
    }
    //! Pop and restore last lower bound from the stack.
    void pop_lower() {
        assert(!lower_bound_stack_.empty());
        lower_bound_ = lower_bound_stack_.back().second;
        lower_bound_stack_.pop_back();
    }
    //! Get the smallest possible value the variable can take.
    [[nodiscard]] auto min_bound() const -> val_t {
        return lower_bound_stack_.empty() ? lower_bound_ : lower_bound_stack_.front().second;
    }

    //! @}

    //! @name Functions for Upper Bounds
    //! @{

    //! Get current upper bound.
    [[nodiscard]] auto upper_bound() const -> val_t { return upper_bound_; }
    //! Set new upper bound.
    void upper_bound(val_t upper_bound) {
        assert(upper_bound <= upper_bound_);
        upper_bound_ = upper_bound;
    }
    //! Push the current upper bound on the stack.
    void push_upper(level_t level) { upper_bound_stack_.emplace_back(level, upper_bound_); }
    //! Check if the given level has already been pushed on the stack.
    [[nodiscard]] auto pushed_upper(level_t level) const -> bool {
        return !upper_bound_stack_.empty() && upper_bound_stack_.back().first == level;
    }
    //! Pop and restore last upper bound from the stack.
    void pop_upper() {
        assert(!upper_bound_stack_.empty());
        upper_bound_ = upper_bound_stack_.back().second;
        upper_bound_stack_.pop_back();
    }
    //! Get the smallest possible value the variable can take.
    [[nodiscard]] auto max_bound() const -> val_t {
        return upper_bound_stack_.empty() ? upper_bound_ : upper_bound_stack_.front().second;
    }

    //! @}

    //! @name Functions for Values and Literals
    //! @{

    //! Get the variable index of the state.
    [[nodiscard]] auto var() const -> var_t { return var_; }

    //! Determine if the variable is assigned, i.e., the current lower bound
    //! equals the current upper bound.
    [[nodiscard]] auto is_assigned() const -> bool { return lower_bound_ == upper_bound_; }

    [[nodiscard]] auto size() const -> val_t { return max_bound() - min_bound(); }

    //! Get a reference to an existing or newly created literal.
    [[nodiscard]] auto get_or_add_literal(val_t value) -> lit_t & {
        if (offset_ == unused && !mogrify_()) {
            return litmap_.emplace(value, 0).first->second;
        }
        return litvec_[value - offset_];
    }

    //! Determine if the given value is associated with an order literal.
    [[nodiscard]] auto has_literal(val_t value) const -> bool {
        if (offset_ == unused) {
            return litmap_.find(value) != litmap_.end();
        }
        return litvec_[value - offset_] != 0;
    }
    //! Get the literal associated with the value.
    [[nodiscard]] auto get_literal(val_t value) -> std::optional<lit_t> {
        if (offset_ == unused) {
            auto it = litmap_.find(value);
            if (it != litmap_.end()) {
                return it->second;
            }
        } else {
            auto ret = litvec_[value - offset_];
            if (ret != 0) {
                return ret;
            }
        }
        return std::nullopt;
    }

    //! Set the literal of the given value.
    void set_literal(val_t value, lit_t lit) {
        if (offset_ == unused && !mogrify_()) {
            litmap_[value] = lit;
        } else {
            litvec_[value - offset_] = lit;
        }
    }
    //! Unset the literal of the given value.
    void unset_literal(val_t value) {
        if (offset_ == unused) {
            litmap_.erase(value);
        } else {
            litvec_[value - offset_] = 0;
        }
    }

    //! @}

    //! @name Functions for Traversing Order Literals
    //! @{

    //! Traverse all literals.
    template <typename F> [[nodiscard]] auto with(F &&f) const {
        if (offset_ == unused) {
            return call_map_(std::forward<F>(f), litmap_.begin(), litmap_.end());
        }
        return call_vec_(std::forward<F>(f), litvec_.begin(), litvec_.end());
    }

    //! Traverse literals preceeding value.
    template <typename F> [[nodiscard]] auto with_lt(val_t value, F &&f) const -> RetType<F, ReverseIteratorVec> {
        if (offset_ == unused) {
            return call_map_(std::forward<F>(f), ReverseIteratorMap{litmap_.lower_bound(value)}, litmap_.rend());
        }
        auto offset = std::min<val_t>(std::max(0, value - offset_), static_cast<val_t>(litvec_.size()));
        return call_vec_(std::forward<F>(f), ReverseIteratorVec{litvec_.begin() + offset}, litvec_.rend());
    }
    //! Traverse literals preceeding and including value.
    template <typename F> [[nodiscard]] auto with_le(val_t value, F &&f) const -> RetType<F, ReverseIteratorVec> {
        if (offset_ == unused) {
            return call_map_(std::forward<F>(f), ReverseIteratorMap{litmap_.upper_bound(value)}, litmap_.rend());
        }
        auto offset = std::min<val_t>(std::max(0, value - offset_ + 1), static_cast<val_t>(litvec_.size()));
        return call_vec_(std::forward<F>(f), ReverseIteratorVec{litvec_.begin() + offset}, litvec_.rend());
    }
    //! Traverse literals succeeding value.
    template <typename F> [[nodiscard]] auto with_gt(val_t value, F &&f) const -> RetType<F, IteratorVec> {
        if (offset_ == unused) {
            return call_map_(std::forward<F>(f), litmap_.upper_bound(value), litmap_.end());
        }
        auto offset = std::min<val_t>(std::max(0, value - offset_ + 1), static_cast<val_t>(litvec_.size()));
        return call_vec_(std::forward<F>(f), litvec_.begin() + offset, litvec_.end());
    }

    //! Traverse literals succeeding and including value.
    template <typename F> [[nodiscard]] auto with_ge(val_t value, F &&f) const -> RetType<F, IteratorVec> {
        if (offset_ == unused) {
            return call_map_(std::forward<F>(f), litmap_.lower_bound(value), litmap_.end());
        }
        auto offset = std::min<val_t>(std::max(0, value - offset_), static_cast<val_t>(litvec_.size()));
        return call_vec_(std::forward<F>(f), litvec_.begin() + offset, litvec_.end());
    }

    //! Get preceeding literal.
    [[nodiscard]] auto lit_lt(val_t value) const -> lit_t {
        return with_lt(value, [](auto ib, auto ie, auto get_lit, auto get_val, auto inc) -> lit_t {
            static_cast<void>(get_val);
            static_cast<void>(inc);
            return ib != ie ? get_lit(ib) : 0;
        });
    }
    //! Get preceeding or equal literal.
    [[nodiscard]] auto lit_le(val_t value) const -> lit_t {
        return with_le(value, [](auto ib, auto ie, auto get_lit, auto get_val, auto inc) -> lit_t {
            static_cast<void>(get_val);
            static_cast<void>(inc);
            return ib != ie ? get_lit(ib) : 0;
        });
    }
    //! Get succeeding literal.
    [[nodiscard]] auto lit_gt(val_t value) const -> lit_t {
        return with_gt(value, [](auto ib, auto ie, auto get_lit, auto get_val, auto inc) -> lit_t {
            static_cast<void>(get_val);
            static_cast<void>(inc);
            return ib != ie ? get_lit(ib) : 0;
        });
    }
    //! Get succeeding or equal literal.
    [[nodiscard]] auto lit_ge(val_t value) const -> lit_t {
        return with_ge(value, [](auto ib, auto ie, auto get_lit, auto get_val, auto inc) -> lit_t {
            static_cast<void>(get_val);
            static_cast<void>(inc);
            return ib != ie ? get_lit(ib) : 0;
        });
    }

    //! Get preceeding order literal.
    [[nodiscard]] auto order_lit_lt(val_t value) const -> OrderLiteralOpt {
        return with_lt(value, [](auto ib, auto ie, auto get_lit, auto get_val, auto inc) -> OrderLiteralOpt {
            static_cast<void>(inc);
            return ib != ie ? OrderLiteralOpt{{get_lit(ib), get_val(ib)}} : std::nullopt;
        });
    }
    //! Get preceeding or equal order literal.
    [[nodiscard]] auto order_lit_le(val_t value) const -> OrderLiteralOpt {
        return with_le(value, [](auto ib, auto ie, auto get_lit, auto get_val, auto inc) -> OrderLiteralOpt {
            static_cast<void>(inc);
            return ib != ie ? OrderLiteralOpt{{get_lit(ib), get_val(ib)}} : std::nullopt;
        });
    }
    //! Get succeeding order literal.
    [[nodiscard]] auto order_lit_gt(val_t value) const -> OrderLiteralOpt {
        return with_gt(value, [](auto ib, auto ie, auto get_lit, auto get_val, auto inc) -> OrderLiteralOpt {
            static_cast<void>(inc);
            return ib != ie ? OrderLiteralOpt{{get_lit(ib), get_val(ib)}} : std::nullopt;
        });
    }
    //! Get succeeding or equal order literal.
    [[nodiscard]] auto order_lit_ge(val_t value) const -> OrderLiteralOpt {
        return with_ge(value, [](auto ib, auto ie, auto get_lit, auto get_val, auto inc) -> OrderLiteralOpt {
            static_cast<void>(inc);
            return ib != ie ? OrderLiteralOpt{{get_lit(ib), get_val(ib)}} : std::nullopt;
        });
    }

    //! Common access pattern involving lit_lt.
    [[nodiscard]] auto lit_prev(val_t value) const -> lit_t {
        auto lit = lit_lt(value);
        return lit != 0 ? lit : -TRUE_LIT;
    }

    //! Common access pattern involving lit_gt.
    [[nodiscard]] auto lit_succ(val_t value) const -> lit_t {
        auto lit = lit_gt(value);
        return lit != 0 ? lit : TRUE_LIT;
    }

    //! @}

  private:
    [[nodiscard]] auto mogrify_() -> bool {
        // Note: The second part of the condition is necessary because the
        // solver might clean up literals at a later point. It is only
        // guaranteed that it will not introduce literals for values out of
        // bounds.
        if (static_cast<val_t>(litmap_.size()) > size() / MOGRIFY_FACTOR && min_bound() <= litmap_.begin()->first &&
            litmap_.rbegin()->first < max_bound()) {
            auto offset = min_bound();
            OrderVec vec(size());
            for (auto [val, lit] : litmap_) {
                vec[val - offset] = lit;
            }
            litmap_.~OrderMap();
            offset_ = offset;
            new (&litvec_) OrderVec(std::move(vec));
            return true;
        }
        return false;
    }

    [[nodiscard]] auto get_val_(IteratorVec it) const -> val_t {
        return static_cast<val_t>(it - litvec_.begin() + offset_);
    }

    [[nodiscard]] val_t get_val_(ReverseIteratorVec it) const { // NOLINT
        return static_cast<val_t>(litvec_.rend() - it + offset_) - 1;
    }

    template <typename F, typename It> auto call_vec_(F &&f, It ib, It ie) const {
        for (; ib != ie && *ib == 0; ++ib) {
        };
        return f(
            ib, ie, [](auto it) { return *it; }, [&](auto it) { return get_val_(it); },
            [ie](auto &it) {
                for (++it; it != ie && *it == 0; ++it) {
                };
            });
    }

    template <typename F, typename It> auto call_map_(F &&f, It ib, It ie) const {
        return f(
            ib, ie, [](auto it) { return it->second; }, [](auto it) { return it->first; }, [](auto &it) { ++it; });
    }

    var_t var_;                    //!< variable associated with the state
    val_t lower_bound_;            //!< current lower bound of the variable
    val_t upper_bound_;            //!< current upper bound of the variable
    val_t offset_{unused};         //!< minimium bound at the time of mogrification
    BoundStack lower_bound_stack_; //!< lower bounds of lower levels
    BoundStack upper_bound_stack_; //!< upper bounds of lower levels
    union {
        OrderMap litmap_; //!< map from values to literals
        OrderVec litvec_; //!< map from values to literals
    };
};

class Solver {
    class Level;
    class LitmapEntry;

  public:
    Solver(SolverConfig const &config, SolverStatistics &stats);

    Solver() = delete;
    Solver(Solver const &x) = delete;
    Solver(Solver &&x) noexcept;
    auto operator=(Solver const &x) -> Solver & = delete;
    auto operator=(Solver &&x) -> Solver & = delete;
    ~Solver();

    //! Get the solver's configuration.
    [[nodiscard]] auto config() const -> SolverConfig const & { return config_; }
    //! Get the solver's statistics.
    [[nodiscard]] auto statistics() -> SolverStatistics & { return stats_; }

    //! Get temporary reason valid until the next call to this function.
    [[nodiscard]] auto temp_reason() -> std::vector<lit_t> & {
        temp_reason_.clear();
        return temp_reason_;
    }

    //! Get the solver's statistics.
    [[nodiscard]] auto statistics() const -> SolverStatistics const & { return stats_; }

    //! Get the Clingcon::VarState object associated with the given variable.
    [[nodiscard]] auto var_state(var_t var) -> VarState & {
        assert(var < var2vs_.size());
        return var2vs_[var];
    }
    //! Get the Clingcon::VarState object associated with the given variable.
    [[nodiscard]] auto var_state(var_t var) const -> VarState const & {
        assert(var < var2vs_.size());
        return var2vs_[var];
    }

    //! Get the Clingcon::ConstraintState object associated with the given constraint.
    [[nodiscard]] auto constraint_state(AbstractConstraint &constraint) -> AbstractConstraintState & {
        return *c2cs_.find(&constraint)->second;
    }

    //! Returns the literal associated with the `vs.var/value` pair.
    //!
    //! Values smaller below the smallest lower bound are associated with the
    //! false literal and values greater or equal to the largest upper bound
    //! with the true literal.
    //!
    //! This function creates a new literal using `cc` if there is no literal
    //! for the given value.
    [[nodiscard]] auto get_literal(AbstractClauseCreator &cc, VarState &vs, val_t value) -> lit_t;

    //! This function is an extended version of Solver::get_literal that can
    //! assign a fact literal if the value did not have a literal before.
    [[nodiscard]] auto update_literal(AbstractClauseCreator &cc, VarState &vs, val_t value, Clingo::TruthValue truth)
        -> lit_t;

    //! Get the current value of a variable.
    //!
    //! This function should be called on the solver corresponding to the thread
    //! where a model has been found.
    [[nodiscard]] auto get_value(var_t var) const -> val_t;

    //! Get the current bound of the minimize constraint.
    [[nodiscard]] auto minimize_bound() const -> std::optional<sum_t>;

    //! Updates the bound of the minimize constraint in this state.
    void update_minimize(AbstractConstraint &constraint, level_t level, sum_t bound);

    //! Watch the given variable notifying given constraint state on changes.
    //!
    //! The integer `i` is additional information passed to the constraint
    //! state upon notification.
    void add_var_watch(var_t var, val_t i, AbstractConstraintState &cs);

    //! Remove a previously added watch.
    void remove_var_watch(var_t var, val_t i, AbstractConstraintState &cs);

    //! Mark a constraint state as inactive.
    void mark_inactive(AbstractConstraintState &cs);

    //! @name Initialization
    //! @{

    //! This function resets a state and should be called when a new solve step is
    //! started.
    //!
    //! This function removes all solve step local variables from the state, maps
    //! fixed global literals to the true/false literal, and resets the minimize
    //! constraint.
    void update();

    //! Integrate the lower and upper bounds from State `other`.
    //!
    //! The function might add clauses to fix literals that have to be updated.
    //! This can lead to a conflict if states have conflicting lower/upper
    //! bounds.
    //!
    //! Precondition: update should be called before this function to really
    //! integrate all bounds.
    [[nodiscard]] auto update_bounds(AbstractClauseCreator &cc, Solver &other, bool check_state) -> bool;

    //! Adds a new VarState object and returns its index;
    [[nodiscard]] auto add_variable(val_t min_int, val_t max_int) -> var_t;

    //! Integrates the given domain for varibale var.
    //!
    //! Consider x in {[1,3), [4,6), [7,9)}. We can simply add the binary
    //! constraints:
    //! - right to left
    //!   - true => x < 9
    //!   - x < 7 => x < 6
    //!   - x < 4 => x < 3
    //! - left to right
    //!   - true => x >= 1
    //!   - x >= 3 => x >= 4
    //!   - x >= 6 => x >= 7
    [[nodiscard]] auto add_dom(AbstractClauseCreator &cc, lit_t lit, var_t var, IntervalSet<val_t> const &domain)
        -> bool;

    //! This function integrates singleton constraints intwatches_o the state.
    //!
    //! We explicitely handle the strict case here to avoid introducing
    //! unnecessary literals.
    [[nodiscard]] auto add_simple(AbstractClauseCreator &cc, lit_t clit, val_t co, var_t var, val_t rhs, bool strict)
        -> bool;

    //! Add the given constraint to the propagation queue and initialize its state.
    auto add_constraint(AbstractConstraint &constraint) -> AbstractConstraintState &;

    //! Remove a constraint.
    void remove_constraint(AbstractConstraint &constraint);

    //! Simplify the state using fixed literals in the trail up to the given
    //! offset and the enqued constraints in the todo list.
    //!
    //! Note that this functions assumes that newly added constraints have been
    //! enqueued before.
    [[nodiscard]] auto simplify(AbstractClauseCreator &cc, bool check_state) -> bool;

    //! Translate constraints in the map l2c and return a list of constraint
    //! added during translation.
    //!
    //! This functions removes translated constraints from the map and the
    //! state. Constraints added during the translation have to be added to the
    //! propagator as well.
    auto translate(InitClauseCreator &cc, Statistics &stats, Config const &conf, ConstraintVec &constraints) -> bool;

    //! Enable translation of minimize constraint.
    //!
    //! Note that the translation of minimize constraints cannot be disabled once enabled.
    //! Furthermore, translation only happens in the master solver.
    void enable_translate_minimize() { translate_minimize_ = true; }

    //! Return true if the minimize constraint has to be translated.
    [[nodiscard]] auto translate_minimize() const -> bool { return translate_minimize_; }

    //! Optimize internal data structures.
    //!
    //! Should be called before solving/copying states.
    void shrink_to_fit();

    //! Copy order literals and propagation state from the given `master` state
    //! to the current state.
    //!
    //! This function must be called on the top level.
    void copy_state(Solver const &master);

    //! Mark the number of static variables.
    //!
    //! This function has to be called at the end of propagator initialization
    //! when all static order variables have been introduced.
    //! The marker can then be used in update() to discard order variables introduced during propagation.
    void mark_variables(var_t var) {
        assert(max_static_var_ <= var);
        max_static_var_ = var;
    }

    //! @}

    //! @name Propagation
    //! @{

    //! Propagates constraints and order literals.
    //!
    //! Constraints that became true are added to the todo list and bounds of
    //! variables are adjusted according to the truth of order literals.
    [[nodiscard]] auto propagate(AbstractClauseCreator &cc, Clingo::LiteralSpan changes) -> bool;

    //! This functions propagates facts that have not been integrated on the
    //! current level and propagates constraints gathered during Solver::propagate.
    [[nodiscard]] auto check(AbstractClauseCreator &cc, bool check_state) -> bool;

    //! This function selects a variable that is not fully assigned w.r.t. the
    //! current assignment and introduces an additional order literal for it.
    //!
    //! This function should only be called total assignments.
    void check_full(AbstractClauseCreator &cc, bool check_solution);

    //! This function undos decision level specific state.
    //!
    //! This includes undoing changed bounds of variables clearing constraints
    //! that where not propagated on the current decision level.
    void undo();

    [[nodiscard]] auto decide(Clingo::Assignment const &assign, lit_t fallback) -> lit_t;
    //! @}

  private:
    //! Update preceeding and succeeding literals of order literal with the
    //! given value.
    auto update_litmap_(VarState &vs, lit_t lit, val_t value) -> std::pair<lit_t, lit_t>;

    //! See Solver::propagate.
    template <class It> [[nodiscard]] auto propagate_(AbstractClauseCreator &cc, It begin, It end) -> bool;

    //! See Solver::propagate.
    [[nodiscard]] auto propagate_(AbstractClauseCreator &cc, lit_t lit) -> bool;

    //! Propagates the preceeding or succeeding order literals of lit until a
    //! true literal is found or the end is reached.
    template <int sign, class It, class L, class I>
    [[nodiscard]] auto propagate_variables_(AbstractClauseCreator &cc, lit_t reason_lit, It begin, It end, L get_lit,
                                            I inc) -> bool;

    //! Update and propgate the given variable due to a lower bound change.
    [[nodiscard]] auto update_lower_(Level &lvl, AbstractClauseCreator &cc, var_t var, lit_t lit, val_t value,
                                     lit_t prev_lit) -> bool;

    //! Update and propgate the given variable due to an upper bound change.
    [[nodiscard]] auto update_upper_(Level &lvl, AbstractClauseCreator &cc, var_t var, lit_t lit, val_t value,
                                     lit_t succ_lit) -> bool;

    //! If the given literal is an order literal, this function updates the lower
    //! or upper bound of the corresponding variables. Furthermore, the preceeding
    //! or succeeding order literals are propagated.
    [[nodiscard]] auto update_domain_(AbstractClauseCreator &cc, lit_t lit) -> bool;

    //! Add a new decision level specific state if necessary.
    //!
    //! Has to be called in Solver::propagate.
    void push_level_(level_t level);

    //! Get the state associated with the current decision level.
    //!
    //! Should only be used in propagate, undo, and check. When check is called,
    //! the current decision level can be higher than that of the Level object
    //! returned. This can only happen when backtracking from a model because the
    //! bound of a minimize constraint changed. In this case, check will just
    //! propagate the minimize constraint and return.
    [[nodiscard]] auto level_() -> Level &;

    //! Helper to access the litmap_ always returning a (possibly invalid) entry.
    auto litmap_at_(lit_t lit) -> LitmapEntry &;
    //! Helper to add an element to the litmap_ making sure to resize the container.
    void litmap_add_(VarState &vs, val_t val, lit_t lit);

    //! Solver configuration.
    SolverConfig const &config_;
    //! Solver statitstics;
    SolverStatistics &stats_;
    //! Vector of all VarState objects.
    std::vector<VarState> var2vs_;
    //! Vector for per decision level state.
    std::vector<Level> levels_;
    //! Map from order literals to a list of var_t, val_t pairs.
    //!
    //! If there is an order literal for `var<=value`, then the pair
    //! `(var,value)` is contained in the map
    std::vector<LitmapEntry> litmap_;
    //! Like litmap but for facts only.
    std::vector<std::tuple<lit_t, var_t, val_t, lit_t>> factmap_;
    //! A mapping from constraint states to constraints.
    std::unordered_map<AbstractConstraint *, UniqueConstraintState> c2cs_;
    //! Watches mapping variables to a constraint state and a constraint
    //! specific integer value.
    std::vector<std::vector<std::pair<val_t, AbstractConstraintState *>>> var_watches_;
    //! Upper bound changes to variables since last check call.
    std::vector<val_t> udiff_;
    //! Set of variables whose upper bounds changed since the last check call.
    std::vector<var_t> in_udiff_;
    //! Lower bound changes to variables since last check call.
    std::vector<val_t> ldiff_;
    //! Set of variables whose lower bounds changed since the last check call.
    std::vector<var_t> in_ldiff_;
    //! Set of constraint states to propagate.
    std::vector<AbstractConstraintState *> todo_;
    //! Map from literals to corresponding constraint states.
    std::unordered_multimap<lit_t, AbstractConstraintState *> lit2cs_;
    //! Set of Clingcon::VarState objects with a modified lower bound.
    std::vector<var_t> undo_lower_;
    //! Set of Clingcon::VarState objects that a modified upper bound.
    std::vector<var_t> undo_upper_;
    //! List of constraint states that can become inactive on the next level.
    std::vector<AbstractConstraintState *> inactive_;
    //! List of variable/coefficient/constraint triples that have been removed
    //! from the Solver::v2cs_ map.
    std::vector<std::tuple<var_t, val_t, AbstractConstraintState *>> removed_var_watches_;
    //! Reason vector to avoid unnecessary allocations.
    std::vector<lit_t> temp_reason_;
    //! Offset to speed up Solver::check_full.
    uint32_t split_last_{0};
    //! Offset to speed up Solver::simplify.
    uint32_t trail_offset_{0};
    //! Current bound of the minimize constraint (if any).
    std::optional<sum_t> minimize_bound_;
    //! The minimize constraint might not have been fully propagated below this
    //! level. See Solver::update_minimize.
    level_t minimize_level_{0};
    //! The number of static variables in the solver.
    var_t max_static_var_{0};
    //! Flag that indicates whether the minimize constraint is to be translated.
    bool translate_minimize_{false};
};

} // namespace Clingcon

#endif // CLINGCON_SOLVER_H
