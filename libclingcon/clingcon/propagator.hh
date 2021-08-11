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

#include <clingcon/solver.hh>
#include <clingcon/constraints.hh>

#include <unordered_map>
#include <unordered_set>
#include <atomic>

//! @file clingcon/propagator.hh
//! This module implements a theory propagator for CSP constraints.
//!
//! @author Roland Kaminski

namespace Clingcon {

using UniqueMinimizeConstraint = std::unique_ptr<MinimizeConstraint>;

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
    Propagator& operator=(Propagator const &) = delete;
    Propagator& operator=(Propagator &&) = delete;
    ~Propagator() override = default;

    //! Return statistics object.
    Statistics const &statistics() {
        return stats_step_;
    }

    Config &config() { return config_; }

    //! Extend the model with the assignment and take care of minimization.
    void on_model(Clingo::Model &model);

    //! Callback to update step and accumulated statistics.
    void on_statistics(Clingo::UserStatistics &step, Clingo::UserStatistics &accu);

    //! Add a variable to the program.
    var_t add_variable(Clingo::Symbol sym);

    //! Enable show statement.
    //!
    //! If the show statement has not been enabled, then all variables are
    //! shown.
    void show() {
        show_ = true;
    }

    //! Show the given variable.
    void show_variable(var_t var);

    //! Show variables with the given signature.
    void show_signature(char const *name, size_t arity);

    //! Add a domain for the given variable.
    [[nodiscard]] bool add_dom(AbstractClauseCreator &cc, lit_t lit, var_t var, IntervalSet<val_t> const &domain);

    //! Add a constraint that can be represented by an order literal.
    [[nodiscard]] bool add_simple(AbstractClauseCreator &cc, lit_t clit, val_t co, var_t var, val_t rhs, bool strict);

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

    [[nodiscard]] lit_t decide(Clingo::id_t thread_id, Clingo::Assignment const &assign, lit_t fallback) override;

    //! Determine if the given variable should be shown.
    [[nodiscard]] bool shown(var_t var);

    //! Get the map from indices to variable names.
    [[nodiscard]] VarMap const &var_map() const {
        return var_map_;
    }

    //! Get the index associated with the given variable index.
    [[nodiscard]] std::optional<var_t> get_index(Clingo::Symbol sym) const;

    //! Get the symbol associated with the given variable index.
    [[nodiscard]] std::optional<Clingo::Symbol> get_symbol(var_t var) const;

    //! Get the value of the variable with the given index in the solver
    //! associated with `thread_id`.
    //!
    //! Should be called on total assignments.
    [[nodiscard]] val_t get_value(var_t var, uint32_t thread_id) const;

    //! Check if the propagator has a minimize constraint.
    [[nodiscard]] bool has_minimize() const {
        return minimize_ != nullptr;
    }
    //! Get a reference to the propagator's minimize constraint.
    MinimizeConstraint const &get_minimize() {
        assert(has_minimize());
        return *minimize_;
    }

    //! Evaluates the minimize constraint w.r.t. the given thread.
    //!
    //! Should be called on total assignments.
    sum_t get_minimize_value(uint32_t thread_id);

    //! Set the `bound` of the minimize constraint.
    void update_minimize(sum_t bound);

    //! Removes the minimize constraint from the lookup lists.
    UniqueMinimizeConstraint remove_minimize();
private:
    //! Get the solver associated with the given `thread_id`.
    [[nodiscard]] Solver &solver_(uint32_t thread_id) {
        assert(thread_id < solvers_.size());
        return solvers_[thread_id];
    }
    //! Get the solver associated with the given `thread_id`.
    [[nodiscard]] Solver const &solver_(uint32_t thread_id) const {
        return solvers_[thread_id];
    }

    //! Get the master config.
    //!
    //! Upon first use this will also apply the default config to the master.
    //! This means that the default config should be modified before the first
    //! functions in this class are accessed.
    Solver &master_() {
        if (solvers_.empty()) {
            solvers_.emplace_back(config_.solver_config(0), stats_step_.solver_stats(0));
        }
        return solver_(0);
    }

    //! Add collected statistics in stats to the clingo's statistics.
    static void add_statistics_(Clingo::UserStatistics &root, Statistics &stats);

    //! Add a constraint to the program that has already been added to the
    //! master solver.
    void add_constraint_(UniqueConstraint constraint);

    //! Propagate constraints refining bounds.
    bool simplify_(AbstractClauseCreator &cc);

    //! Translates constraints and take care of handling the minimize
    //! constraint.
    bool translate_(InitClauseCreator &cc, UniqueMinimizeConstraint minimize);

    //! Add a new minimize constraint.
    //!
    //! There must only be one active minimize constraints. Any previous
    //! minimize constraints have to be removed first.
    void add_minimize_(UniqueMinimizeConstraint minimize);

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
    bool show_{false};                            //!< whether there is a show statement
};

} // namespace Clingcon

#endif // CLINGCON_PROPAGATOR_H
