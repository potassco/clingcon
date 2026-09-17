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

#ifndef CLINGCON_PROPAGATOR_H
#define CLINGCON_PROPAGATOR_H

#include <clingcon/constraints.hh>
#include <clingcon/solver.hh>

#include <atomic>
#include <set>
#include <unordered_map>
#include <unordered_set>

//! @file clingcon/propagator.hh
//! This module implements a theory propagator for CSP constraints.
//!
//! @author Roland Kaminski

namespace Clingcon {

using UniqueMinimizeConstraint = std::unique_ptr<MinimizeConstraint>;

//! Message for options that need more than one solve call.
constexpr char const *STEPS_REQUIRED_MESSAGE = "--lex-opt=multi-shot optimizes one level per solve call; "
                                               "call clingcon_enable_steps before the first solve call and "
                                               "clingcon_step after every solve call";

//! A propagator for CSP constraints.
class Propagator final : public Clingo::Heuristic {
  public:
    using VarMap = std::map<var_t, Clingo::Symbol>;
    using SymMap = std::unordered_map<Clingo::Symbol, var_t>;
    using VarSet = std::unordered_set<var_t>;
    using SigSet = std::unordered_set<Clingo::Signature>;

    Propagator() = default;
    Propagator(Propagator const &) = delete;
    Propagator(Propagator &&) = delete;
    auto operator=(Propagator const &) -> Propagator & = delete;
    auto operator=(Propagator &&) -> Propagator & = delete;
    ~Propagator() override;

    //! Return statistics object.
    auto statistics() -> Statistics const & { return stats_step_; }

    auto config() -> Config & { return config_; }

    //! Extend the model with the assignment and take care of minimization.
    void on_model(Clingo::Model &model);

    //! Callback to update step and accumulated statistics.
    void on_statistics(Clingo::UserStatistics &step, Clingo::UserStatistics &accu);

    //! Add a variable to the program.
    auto add_variable(Clingo::Symbol sym) -> var_t;

    //! Enable show statement.
    //!
    //! If the show statement has not been enabled, then all variables are
    //! shown.
    void show() { show_ = true; }

    //! Show the given variable.
    void show_variable(var_t var);

    //! Show variables with the given signature.
    void show_signature(char const *name, size_t arity);

    //! Add a domain for the given variable.
    [[nodiscard]] auto add_dom(AbstractClauseCreator &cc, lit_t lit, var_t var, IntervalSet<val_t> const &domain)
        -> bool;

    //! Add a constraint that can be represented by an order literal.
    [[nodiscard]] auto add_simple(AbstractClauseCreator &cc, lit_t clit, val_t co, var_t var, val_t rhs, bool strict)
        -> bool;

    //! Add a constraint to the program.
    void add_constraint(UniqueConstraint constraint);

    //! Initializes the propagator extracting constraints from the theory data.
    //!
    //! The function handles reinitialization for multi-shot solving and
    //! multi-threaded solving.
    void init(Clingo::PropagateInit &init) override;

    //! Delegates propagation to the respective solver.
    void propagate(Clingo::PropagateControl &control, Clingo::LiteralSpan changes) override;

    //! Delegates checking to the respective solver and makes sure that all
    //! order variables are assigned if the assigment is total.
    void check(Clingo::PropagateControl &control) override;

    //! Delegates undoing to the respective solver.
    void undo(Clingo::PropagateControl const &control, Clingo::LiteralSpan changes) noexcept override;

    [[nodiscard]] auto decide(Clingo::id_t thread_id, Clingo::Assignment const &assign, lit_t fallback)
        -> lit_t override;

    //! Determine if the given variable should be shown.
    [[nodiscard]] auto shown(var_t var) -> bool;

    //! Get the map from indices to variable names.
    [[nodiscard]] auto var_map() const -> VarMap const & { return var_map_; }

    //! Get the index associated with the given variable index.
    [[nodiscard]] auto get_index(Clingo::Symbol sym) const -> std::optional<var_t>;

    //! Get the symbol associated with the given variable index.
    [[nodiscard]] auto get_symbol(var_t var) const -> std::optional<Clingo::Symbol>;

    //! Get the value of the variable with the given index in the solver
    //! associated with `thread_id`.
    //!
    //! Should be called on total assignments.
    [[nodiscard]] auto get_value(var_t var, uint32_t thread_id) const -> val_t;

    //! Check if the propagator has a minimize constraint.
    [[nodiscard]] auto has_minimize() const -> bool { return minimize_ != nullptr; }
    //! Get a reference to the propagator's minimize constraint.
    auto get_minimize() -> MinimizeConstraint const & {
        assert(has_minimize());
        return *minimize_;
    }

    //! Evaluates the minimize constraint w.r.t. the given thread.
    //!
    //! Should be called on total assignments.
    auto get_minimize_value(uint32_t thread_id) -> sum_t;

    //! Set the `bound` of the minimize constraint.
    void update_minimize(sum_t bound);

    //! Removes the minimize constraint from the lookup lists.
    auto remove_minimize() -> UniqueMinimizeConstraint;

    //! @name Optimization in several solve calls
    //! @{

    //! Announce that the application calls Propagator::step after each solve
    //! call.
    void enable_steps() { steps_enabled_ = true; }

    //! Check whether the application drives the optimization.
    [[nodiscard]] auto steps_enabled() const -> bool { return steps_enabled_; }

    //! Prepare the next solve call.
    //!
    //! This finishes the active optimization level and activates the next one.
    //! It returns true if there is another level to optimize. A solve call
    //! that was cut short, because a limit on models or time was reached or
    //! the search was interrupted, has not finished its level; `exhausted`
    //! says whether the call ran to completion. Calling this function again
    //! before the next solve call does nothing.
    auto step(bool exhausted) -> bool;

    //! @}

    //! @name Multi-level optimization
    //! @{

    //! One level of a lexicographic optimization.
    struct MinimizeLevel {
        UniqueMinimizeConstraint constraint; //!< the constraint of the level while it is not active
        MinimizeConstraint *ptr{nullptr};    //!< the constraint of the level
        int priority{0};                     //!< the level given after `@`
        sum_t locked_value{0};               //!< the optimum of the level once it is locked
        bool locked{false};                  //!< whether the level has been optimized
        bool lock_added{false};              //!< whether the constraint fixing the optimum has been added
    };

    //! Check whether the minimize constraint has more than one level.
    [[nodiscard]] auto has_multi_level() const -> bool { return !minimize_levels_.empty(); }

    //! Get the number of optimization levels.
    [[nodiscard]] auto num_levels() const -> size_t { return minimize_levels_.size(); }

    //! Get the value of each level in the last model.
    [[nodiscard]] auto level_costs() const -> std::vector<sum_t> const & { return level_costs_; }

    //! @}

  private:
    //! Get the solver associated with the given `thread_id`.
    [[nodiscard]] auto solver_(uint32_t thread_id) -> Solver & {
        assert(thread_id < solvers_.size());
        return solvers_[thread_id];
    }
    //! Get the solver associated with the given `thread_id`.
    [[nodiscard]] auto solver_(uint32_t thread_id) const -> Solver const & { return solvers_[thread_id]; }

    //! Get the master config.
    //!
    //! Upon first use this will also apply the default config to the master.
    //! This means that the default config should be modified before the first
    //! functions in this class are accessed.
    auto master_() -> Solver & {
        if (solvers_.empty()) {
            solvers_.emplace_back(config_.solver_config(0), stats_step_.solver_stats(0));
        }
        return solver_(0);
    }

    //! Add collected statistics in stats to the clingo's statistics.
    static void add_statistics_(Clingo::UserStatistics &root, Statistics &stats);

    //! Add the statistics of the optimization levels to clingo's statistics.
    void add_level_statistics_(Clingo::UserStatistics &root);

    //! Add a constraint to the program that has already been added to the
    //! master solver.
    void add_constraint_(UniqueConstraint constraint);

    //! Propagate constraints refining bounds.
    auto simplify_(AbstractClauseCreator &cc) -> bool;

    //! Translates constraints and take care of handling the minimize
    //! constraint.
    auto translate_(InitClauseCreator &cc, UniqueMinimizeConstraint minimize) -> bool;

    //! Add a new minimize constraint.
    //!
    //! There must only be one active minimize constraints. Any previous
    //! minimize constraints have to be removed first.
    void add_minimize_(UniqueMinimizeConstraint minimize);

    //! Lock the active level at its optimum and activate the next one.
    //!
    //! Returns true if there is another level to optimize.
    auto advance_level() -> bool;

    //! Value of minimize constraint, when no bound has been found yet.
    //!
    //! Since this is the largest value a minimize constraint can take, all
    //! models found will have a value less than or equal to it.
    static constexpr sum_t no_bound = std::numeric_limits<sum_t>::max();

    Config config_;                               //!< configuration
    ConstraintVec constraints_;                   //!< the set of constraints
    std::vector<Solver> solvers_;                 //!< map thread id to solvers
    SymMap sym_map_;                              //!< map from variable names to indices
    VarMap var_map_;                              //!< map from indices to variable names
    Statistics stats_step_;                       //!< statistics of the current call
    Statistics stats_accu_;                       //!< accumulated statistics
    VarSet show_variable_;                        //!< variables to show
    SigSet show_signature_;                       //!< signatures to show
    MinimizeConstraint *minimize_{nullptr};       //!< minimize constraint
    std::atomic<sum_t> minimize_bound_{no_bound}; //!< bound of the minimize constraint
    //! Cost of the last model.
    //!
    //! Propagator::on_statistics clears the statistics of the solve call
    //! before the application calls Propagator::step, which needs the cost to
    //! lock the level.
    std::optional<sum_t> last_known_cost_;
    std::vector<MinimizeLevel> minimize_levels_; //!< the levels, highest priority first
    std::set<int> minimize_levels_seen_;         //!< levels of the elements added to the minimize constraint so far
    std::vector<sum_t> level_costs_;             //!< value of each level in the last model
    uint32_t active_level_idx_{0};               //!< the level being optimized
    bool has_model_{false};                      //!< whether a model has been found in any solve call
    bool steps_enabled_{false};                  //!< whether the application calls Propagator::step
    bool step_due_{false};                       //!< whether the last solve call has to be followed by a step
    bool show_{false};                           //!< whether there is a show statement
};

} // namespace Clingcon

#endif // CLINGCON_PROPAGATOR_H
