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

//! Base class of all constraints.
class AbstractConstraint {
public:
    AbstractConstraint() = default;
    AbstractConstraint(AbstractConstraint const &) = delete;
    AbstractConstraint(AbstractConstraint &&) = delete;
    AbstractConstraint &operator=(AbstractConstraint const &) = delete;
    AbstractConstraint &operator=(AbstractConstraint &&) = delete;
    virtual ~AbstractConstraint() = default;

    //! Create thread specific state for the constraint.
    [[nodiscard]] virtual UniqueConstraintState create_state() = 0;

    //! Get the literal associated with the constraint.
    [[nodiscard]] virtual lit_t literal() const = 0;
};


//! Abstract class to capture the state of constraints.
class AbstractConstraintState {
public:
    AbstractConstraintState() = default;
    AbstractConstraintState(AbstractConstraintState const &) = delete;
    AbstractConstraintState(AbstractConstraintState &&) = delete;
    AbstractConstraintState &operator=(AbstractConstraintState const &) = delete;
    AbstractConstraintState &operator=(AbstractConstraintState &&) = delete;
    virtual ~AbstractConstraintState() = default;

    //! Get the associated constraint.
    virtual AbstractConstraint& constraint() = 0;

    //! @name Initialization Functions
    //! @{

    //! Attach the constraint to a solver.
    virtual void attach(Solver &solver) = 0;
    //! Detach the constraint from a solver.
    virtual void detach(Solver &solver) = 0;
    //! Translate a constraint to simpler constraints.
    [[nodiscard]] virtual std::pair<bool, bool> translate(Config const &config, Solver &solver, InitClauseCreator &cc, ConstraintVec &added) = 0;
    //! Copy the constraint state (for another solver)
    [[nodiscard]] virtual UniqueConstraintState copy() const = 0;

    //! @}

    //! @name Functions for Constraint Progation
    //! @{

    //! Inform the solver about updated bounds of a variable.
    //!
    //! Value i depends on the value passed when registering the watch and diff
    //! is the change to the bound of the watched variable.
    [[nodiscard]] virtual bool update(val_t i, val_t diff) = 0;
    //! Similar to update but when the bound of a variable is backtracked.
    virtual void undo(val_t i, val_t diff) = 0;
    //! Prepagates the constraint.
    [[nodiscard]] virtual bool propagate(Solver &solver, AbstractClauseCreator &cc, bool check_state) = 0;
    //! Check if the solver meets the state invariants.
    virtual void check_full(Solver &solver) = 0;

    //! @}

    //! @name Functions to Manage Todo Sets
    //! @{

    //! Mark the constraint state as todo item.
    virtual bool mark_todo(bool todo) = 0;
    //! Returns true if the constraint is marked as todo item.
    [[nodiscard]] virtual bool marked_todo() const = 0;

    //! @}

    //! @name Functions to Manage Sets of Inactive Constraint States (Template)
    //! @{

    //! Returns true if the constraint is marked inactive.
    [[nodiscard]] bool marked_inactive() const {
        return inactive_level() > 0;
    }
    //! Returns true if the constraint is removable.
    [[nodiscard]] virtual bool removable() = 0;
    //! Mark a constraint inactive on the given level.
    void mark_inactive(level_t level) {
        assert(!marked_inactive());
        inactive_level(level + 1);
    }
    //! Mark a constraint active.
    void mark_active() {
        inactive_level(0);
    }
    //! A constraint is removable if it has been marked inactive on a lower
    //! level.
    [[nodiscard]] bool removable(level_t level) {
        return marked_inactive() && inactive_level() <= level;
    }

    //! @}

protected:
    //! @name Functions Manage Sets of Inactive Constraint States (Virtual)
    //! @{

    //! Get the level on which the constraint became inactive.
    [[nodiscard]] virtual level_t inactive_level() const = 0;
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
public:
    using OrderLiterals = std::map<val_t, lit_t>;                  //!< Container to store order literals.
    using Iterator = OrderLiterals::const_iterator;                //!< Iterator over order literals.
    using ReverseIterator = OrderLiterals::const_reverse_iterator; //!< Reverse iterator over order literals.

    //! Create an initial state for the given variable.
    //!
    //! Initially, the state should have  a lower bound of `Config::min_int`
    //! and an upper bound of `Config::max_int` and is associated with no
    //! variables.
    VarState(var_t var, val_t lower_bound, val_t upper_bound)
    : var_{var}
    , lower_bound_{lower_bound}
    , upper_bound_{upper_bound} {
    }
    VarState() = delete;
    VarState(VarState const &) = default;
    VarState(VarState &&) noexcept = default;
    VarState &operator=(VarState const &) = default;
    VarState &operator=(VarState &&) noexcept = default;
    ~VarState() = default;

    //! Remove all literals associated with this state.
    void reset(val_t min_int, val_t max_int) {
        lower_bound_ = min_int;
        upper_bound_ = max_int;
        lower_bound_stack_.clear();
        upper_bound_stack_.clear();
        literals_.clear();
    }

    //! @name Functions for Lower Bounds
    //! @{

    //! Get current lower bound.
    [[nodiscard]] val_t lower_bound() const {
        return lower_bound_;
    }
    //! Set new lower bound.
    void lower_bound(val_t lower_bound) {
        assert(lower_bound >= lower_bound_);
        lower_bound_ = lower_bound;
    }
    //! Push the current lower bound on the stack.
    void push_lower(level_t level) {
        lower_bound_stack_.emplace_back(level, lower_bound_);
    }
    //! Check if the given level has already been pushed on the stack.
    [[nodiscard]] bool pushed_lower(level_t level) const {
        return !lower_bound_stack_.empty() && lower_bound_stack_.back().first == level;
    }
    //! Pop and restore last lower bound from the stack.
    void pop_lower() {
        assert(!lower_bound_stack_.empty());
        lower_bound_ = lower_bound_stack_.back().second;
        lower_bound_stack_.pop_back();
    }
    //! Get the smallest possible value the variable can take.
    [[nodiscard]] val_t min_bound() const {
        return lower_bound_stack_.empty() ? lower_bound_ : lower_bound_stack_.front().second;
    }

    //! @}

    //! @name Functions for Upper Bounds
    //! @{

    //! Get current upper bound.
    [[nodiscard]] val_t upper_bound() const {
        return upper_bound_;
    }
    //! Set new upper bound.
    void upper_bound(val_t upper_bound) {
        assert(upper_bound <= upper_bound_);
        upper_bound_ = upper_bound;
    }
    //! Push the current upper bound on the stack.
    void push_upper(level_t level) {
        upper_bound_stack_.emplace_back(level, upper_bound_);
    }
    //! Check if the given level has already been pushed on the stack.
    [[nodiscard]] bool pushed_upper(level_t level) const {
        return !upper_bound_stack_.empty() && upper_bound_stack_.back().first == level;
    }
    //! Pop and restore last upper bound from the stack.
    void pop_upper() {
        assert(!upper_bound_stack_.empty());
        upper_bound_ = upper_bound_stack_.back().second;
        upper_bound_stack_.pop_back();
    }
    //! Get the smallest possible value the variable can take.
    [[nodiscard]] val_t max_bound() const {
        return upper_bound_stack_.empty() ? upper_bound_ : upper_bound_stack_.front().second;
    }

    //! @}

    //! @name Functions for Values and Literals
    //! @{

    //! Get the variable index of the state.
    [[nodiscard]] var_t var() const {
        return var_;
    }

    //! Determine if the variable is assigned, i.e., the current lower bound
    //! equals the current upper bound.
    [[nodiscard]] bool is_assigned() const {
        return lower_bound_ == upper_bound_;
    }

    //! Get a reference to an existing or newly created literal.
    [[nodiscard]] lit_t &get_or_add_literal(val_t value) {
        return literals_.emplace(value, 0).first->second;
    }

    //! Determine if the given value is associated with an order literal.
    [[nodiscard]] bool has_literal(val_t value) const {
        return literals_.find(value) != literals_.end();
    }
    //! Get the literal associated with the value.
    [[nodiscard]] std::optional<lit_t> get_literal(val_t value) {
        auto it = literals_.find(value);
        if (it != literals_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    //! Set the literal of the given value.
    void set_literal(val_t value, lit_t lit) {
        literals_[value] = lit;
    }
    //! Unset the literal of the given value.
    void unset_literal(val_t value) {
        literals_.erase(value);
    }

    //! @}

    //! @name Functions for Traversing Order Literals
    //! @{

    //! Return an iterator to the first order literal.
    [[nodiscard]] Iterator begin() const {
        return literals_.begin();
    }
    //! Return an iterator after the last order literal.
    [[nodiscard]] Iterator end() const {
        return literals_.end();
    }

    //! Return a reverse iterator to the last order literal.
    [[nodiscard]] ReverseIterator rbegin() const {
        return literals_.rbegin();
    }
    //! Return a reverse iterator before the first order literal.
    [[nodiscard]] ReverseIterator rend() const {
        return literals_.rend();
    }

    //! Return a reverse iterator to the first order literal with a value less
    //! than the given value.
    [[nodiscard]] ReverseIterator lit_lt(val_t value) {
        return ReverseIterator{literals_.lower_bound(value)};
    }
    //! Return a reverse iterator to the first order literal with a value less
    //! than or equal to the given value.
    [[nodiscard]] ReverseIterator lit_le(val_t value) {
        return ReverseIterator{literals_.upper_bound(value)};
    }

    //! Return an iterator to the first order literal with a value greater than
    //! the given value.
    [[nodiscard]] Iterator lit_gt(val_t value) {
        return literals_.upper_bound(value);
    }
    //! Return an iterator to the first order literal with a value greater than
    //! or equal to the given value.
    [[nodiscard]] Iterator lit_ge(val_t value) {
        return literals_.lower_bound(value);
    }

    //! @}

private:
    using BoundStack = std::vector<std::pair<level_t, val_t>>;

    var_t var_;                    //!< variable associated with the state
    val_t lower_bound_;            //!< current lower bound of the variable
    val_t upper_bound_;            //!< current upper bound of the variable
    BoundStack lower_bound_stack_; //!< lower bounds of lower levels
    BoundStack upper_bound_stack_; //!< upper bounds of lower levels
    OrderLiterals literals_;       //!< map from values to literals
};

class Solver {
    class Level;

public:
    Solver(SolverConfig const &config, SolverStatistics &stats);

    Solver() = delete;
    Solver(Solver const &x) = delete;
    Solver(Solver && x) noexcept;
    Solver& operator=(Solver const &x) = delete;
    Solver& operator=(Solver &&x) = delete;
    ~Solver();

    //! Get the solver's configuration.
    [[nodiscard]] SolverConfig const &config() const {
        return config_;
    }
    //! Get the solver's statistics.
    [[nodiscard]] SolverStatistics &statistics() {
        return stats_;
    }

    //! Get temporary reason valid until the next call to this function.
    [[nodiscard]] std::vector<lit_t> &temp_reason() {
        temp_reason_.clear();
        return temp_reason_;
    }

    //! Get the solver's statistics.
    [[nodiscard]] SolverStatistics const &statistics() const {
        return stats_;
    }

    //! Get the Clingcon::VarState object associated with the given variable.
    [[nodiscard]] VarState &var_state(var_t var) {
        assert(var < var2vs_.size());
        return var2vs_[var];
    }
    //! Get the Clingcon::VarState object associated with the given variable.
    [[nodiscard]] VarState const &var_state(var_t var) const {
        assert(var < var2vs_.size());
        return var2vs_[var];
    }

    //! Get the Clingcon::ConstraintState object associated with the given constraint.
    [[nodiscard]] AbstractConstraintState &constraint_state(AbstractConstraint &constraint) {
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
    [[nodiscard]] lit_t get_literal(AbstractClauseCreator &cc, VarState &vs, val_t value);

    //! This function is an extended version of Solver::get_literal that can
    //! assign a fact literal if the value did not have a literal before.
    [[nodiscard]] lit_t update_literal(AbstractClauseCreator &cc, VarState &vs, val_t value, Clingo::TruthValue truth);

    //! Get the current value of a variable.
    //!
    //! This function should be called on the solver corresponding to the thread
    //! where a model has been found.
    [[nodiscard]] val_t get_value(var_t var) const;

    //! Get the current bound of the minimize constraint.
    [[nodiscard]] std::optional<sum_t> minimize_bound() const;

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
    void update(AbstractClauseCreator &cc);

    //! Integrate the lower and upper bounds from State `other`.
    //!
    //! The function might add clauses to fix literals that have to be updated.
    //! This can lead to a conflict if states have conflicting lower/upper
    //! bounds.
    //!
    //! Precondition: update should be called before this function to really
    //! integrate all bounds.
    [[nodiscard]] bool update_bounds(AbstractClauseCreator &cc, Solver &other, bool check_state);

    //! Adds a new VarState object and returns its index;
    [[nodiscard]] var_t add_variable(val_t min_int, val_t max_int);

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
    [[nodiscard]] bool add_dom(AbstractClauseCreator &cc, lit_t lit, var_t var, IntervalSet<val_t> const &domain);

    //! This function integrates singleton constraints intwatches_o the state.
    //!
    //! We explicitely handle the strict case here to avoid introducing
    //! unnecessary literals.
    [[nodiscard]] bool add_simple(AbstractClauseCreator &cc, lit_t clit, val_t co, var_t var, val_t rhs, bool strict);

    //! Add the given constraint to the propagation queue and initialize its state.
    AbstractConstraintState &add_constraint(AbstractConstraint &constraint);

    //! Remove a constraint.
    void remove_constraint(AbstractConstraint &constraint);

    //! Simplify the state using fixed literals in the trail up to the given
    //! offset and the enqued constraints in the todo list.
    //!
    //! Note that this functions assumes that newly added constraints have been
    //! enqueued before.
    [[nodiscard]] bool simplify(AbstractClauseCreator &cc, bool check_state);

    //! Translate constraints in the map l2c and return a list of constraint
    //! added during translation.
    //!
    //! This functions removes translated constraints from the map and the
    //! state. Constraints added during the translation have to be added to the
    //! propagator as well.
    bool translate(InitClauseCreator &cc, Statistics &stats, Config const &conf, ConstraintVec &constraints);

    //! Optimize internal data structures.
    //!
    //! Should be called before solving/copying states.
    void shrink_to_fit();

    //! Copy order literals and propagation state from the given `master` state
    //! to the current state.
    //!
    //! This function must be called on the top level.
    void copy_state(Solver const &master);

    //! @}

    //! @name Propagation
    //! @{

    //! Propagates constraints and order literals.
    //!
    //! Constraints that became true are added to the todo list and bounds of
    //! variables are adjusted according to the truth of order literals.
    [[nodiscard]] bool propagate(AbstractClauseCreator &cc, Clingo::LiteralSpan changes);

    //! This functions propagates facts that have not been integrated on the
    //! current level and propagates constraints gathered during Solver::propagate.
    [[nodiscard]] bool check(AbstractClauseCreator &cc, bool check_state);

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

    //! @}

private:
    //! See Solver::propagate.
    template <class It>
    [[nodiscard]] bool propagate_(AbstractClauseCreator &cc, It begin, It end);

    //! See Solver::propagate.
    [[nodiscard]] bool propagate_(AbstractClauseCreator &cc, lit_t lit);

    //! Propagates the preceeding or succeeding order literals of lit until a
    //! true literal is found or the end is reached.
    template <int sign, class It>
    [[nodiscard]] bool propagate_variables_(AbstractClauseCreator &cc, lit_t reason_lit, It begin, It end);

    //! Update and propgate the given variable due to a lower bound change.
    [[nodiscard]] bool update_lower_(Level &lvl, AbstractClauseCreator &cc, var_t var, lit_t lit, val_t value);

    //! Update and propgate the given variable due to an upper bound change.
    [[nodiscard]] bool update_upper_(Level &lvl, AbstractClauseCreator &cc, var_t var, lit_t lit, val_t value);

    //! If the given literal is an order literal, this function updates the lower
    //! or upper bound of the corresponding variables. Furthermore, the preceeding
    //! or succeeding order literals are propagated.
    [[nodiscard]] bool update_domain_(AbstractClauseCreator &cc, lit_t lit);

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
    [[nodiscard]] Level &level_();

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
    std::unordered_multimap<lit_t, std::pair<var_t, val_t>> litmap_;
    //! Like litmap but for facts only.
    std::vector<std::tuple<lit_t, var_t, val_t>> factmap_;
    //! A mapping from constraint states to constraints.
    std::unordered_map<AbstractConstraint*, UniqueConstraintState> c2cs_;
    //! Watches mapping variables to a constraint state and a constraint
    //! specific integer value.
    std::vector<std::vector<std::pair<val_t, AbstractConstraintState*>>> var_watches_;
    //! Upper bound changes to variables since last check call.
    std::vector<val_t> udiff_;
    //! Set of variables whose upper bounds changed since the last check call.
    std::vector<var_t> in_udiff_;
    //! Lower bound changes to variables since last check call.
    std::vector<val_t> ldiff_;
    //! Set of variables whose lower bounds changed since the last check call.
    std::vector<var_t> in_ldiff_;
    //! Set of constraint states to propagate.
    std::vector<AbstractConstraintState*> todo_;
    //! Map from literals to corresponding constraint states.
    std::unordered_multimap<lit_t, AbstractConstraintState*> lit2cs_;
    //! Set of Clingcon::VarState objects with a modified lower bound.
    std::vector<var_t> undo_lower_;
    //! Set of Clingcon::VarState objects that a modified upper bound.
    std::vector<var_t> undo_upper_;
    //! List of constraint states that can become inactive on the next level.
    std::vector<AbstractConstraintState*> inactive_;
    //! List of variable/coefficient/constraint triples that have been removed
    //! from the Solver::v2cs_ map.
    std::vector<std::tuple<var_t, val_t, AbstractConstraintState*>> removed_var_watches_;
    //! Offset to speed up Solver::check_full.
    uint32_t split_last_{0};
    //! Offset to speed up Solver::simplify.
    uint32_t trail_offset_{0};
    //! Current bound of the minimize constraint (if any).
    std::optional<sum_t> minimize_bound_;
    //! The minimize constraint might not have been fully propagated below this
    //! level. See Solver::update_minimize.
    level_t minimize_level_{0};
    //! Reason vector to avoid unnecessary allocations.
    std::vector<lit_t> temp_reason_;
};

} // namespace Clingcon

#endif // CLINGCON_SOLVER_H
