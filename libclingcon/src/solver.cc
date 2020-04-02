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

#include "clingcon/solver.hh"
#include "clingcon/util.hh"

#include <unordered_set>

namespace Clingcon {

//! Class that helps to maintain per decision level state.
class Solver::Level {
public:
    Level(level_t level)
    : level_{level} {
    }

    [[nodiscard]] level_t level() const {
        return level_;
    }

    //! Update the lower bound of a var state.
    void update_lower(Solver &solver, VarState &vs, val_t value) {
        val_t diff = value + 1 - vs.lower_bound();
        if (level_ > 0 && !vs.pushed_lower(level_) ) {
            vs.push_lower(level_);
            undo_lower_.emplace_back(vs.var());
        }
        vs.lower_bound(value + 1);

        if (solver.ldiff_[vs.var()] == 0) {
            solver.in_ldiff_.emplace_back(vs.var());
        }
        solver.ldiff_[vs.var()] += diff;
    }

    //! Update the upper bound of a var state.
    void update_upper(Solver &solver, VarState &vs, val_t value) {
        val_t diff = value - vs.upper_bound();

        if (level_ > 0 && !vs.pushed_upper(level_) ) {
            vs.push_upper(level_);
            undo_upper_.emplace_back(vs.var());
        }
        vs.upper_bound(value);

        if (solver.udiff_[vs.var()] == 0) {
            solver.in_udiff_.emplace_back(vs.var());
        }
        solver.udiff_[vs.var()] += diff;
    }

    //! Update watches and enque constraints.
    //!
    //! The parameters determine whether the lookup tables for lower or upper
    //! bounds are used.
    void update_constraints_(Solver &solver, var_t var, val_t diff) {
        auto &watches = solver.var_watches_[var];
        watches.erase(std::remove_if(watches.begin(), watches.end(), [&](auto const &value_cs) {
            if (!value_cs.second->removable(level_)) {
                if (value_cs.second->update(value_cs.first, diff)) {
                    Level::mark_todo(solver, *value_cs.second);
                }
                return false;
            }
            removed_var_watches_.emplace_back(var, value_cs.first, value_cs.second);
            return true;
        }), watches.end());
    }

    //! Mark a constraint state as inactive.
    void mark_inactive(AbstractConstraintState &cs) {
        if (cs.removable() && !cs.marked_inactive()) {
            inactive_.emplace_back(&cs);
            cs.mark_active();
        }
    }

    //! Add the given constraint state to the todo list if it is not yet
    //! contained.
    static void mark_todo(Solver &solver, AbstractConstraintState &cs) {
        if (!cs.mark_todo(true)) {
            solver.todo_.emplace_back(&cs);
        }
    }

    //! This function undos decision level specific state.
    //!
    //! This includes undoing changed bounds of variables clearing constraints
    //! that where not propagated on the current decision level.
    void undo(Solver &solver) {
        // undo lower bound changes
        for (auto var : undo_lower_) {
            auto &vs = solver.var_state(var);
            auto value = vs.lower_bound();
            vs.pop_lower();
            auto diff = value - vs.lower_bound() - solver.ldiff_[var];
            if (diff != 0) {
                for (auto &[co, cs] : solver.var_watches_[var]) {
                    cs->undo(co, diff);
                }
            }
            solver.ldiff_[var] = 0;
        }
        solver.in_ldiff_.clear();

        // undo upper bound changes
        for (auto var : undo_upper_) {
            auto &vs = solver.var_state(var);
            auto value = vs.upper_bound();
            vs.pop_upper();
            auto diff = value - vs.upper_bound() - solver.udiff_[var];
            if (diff != 0) {
                for (auto &[co, cs] : solver.var_watches_[var]) {
                    cs->undo(co, diff);
                }
            }
            solver.udiff_[var] = 0;
        }
        solver.in_udiff_.clear();

        // mark constraints as active again
        for (auto *cs : inactive_) {
            cs->mark_active();
        }

        // add removed watches
        for (auto &[var, val, cs] : removed_var_watches_) {
            solver.var_watches_[var].emplace_back(val, cs);
        }

        // clear remaining todo items
        for (auto *cs : solver.todo_) {
            cs->mark_todo(false);
        }
        solver.todo_.clear();
    }

    //! Remove the constraint state from the propagation state.
    void remove_constraint(Solver &solver, AbstractConstraintState &cs) {
        if (cs.marked_inactive()) {
            cs.mark_active();
            inactive_.erase(std::find(inactive_.begin(), inactive_.end(), &cs));
        }
        if (cs.marked_todo()) {
            cs.mark_todo(false);
            solver.todo_.erase(std::find(solver.todo_.begin(), solver.todo_.end(), &cs));
        }
    }

    //! Remove a set of constraints from the propagation state.
    template <class F>
    void remove_constraints(Solver &solver, F in_removed) {
        for (auto &watches : solver.var_watches_) {
            watches.erase(std::remove_if(watches.begin(), watches.end(), [in_removed](auto &watch) {
                return in_removed(*watch.second);
            }), watches.end());
        }

        inactive_.erase(std::remove_if(inactive_.begin(), inactive_.end(), [in_removed](auto *cs) {
            cs->mark_active();
            return in_removed(*cs);
        }), inactive_.end());

        solver.todo_.erase(std::remove_if(solver.todo_.begin(), solver.todo_.end(), [in_removed](auto *cs) {
            cs->mark_todo(false);
            return in_removed(*cs);
        }), solver.todo_.end());
    }

    //! Copy the given level.
    //!
    //! This function must only be called on the top level. It does not update
    //! variable and constraint states. This has to happen in
    //! Solver::copy_state.
    static void copy_state(Solver &solver, Solver const &master) {
        assert(solver.levels_.size() == 1 && master.levels_.size() == 1);

        auto &lvl = solver.levels_.front();
        auto const &lvl_master = master.levels_.front();

        // copy bound changes
        lvl.undo_lower_.clear();
        for (auto var : lvl_master.undo_lower_) {
            lvl.undo_lower_.emplace_back(var);
        }
        solver.ldiff_ = master.ldiff_;
        solver.in_ldiff_ = master.in_ldiff_;

        lvl.undo_upper_.clear();
        for (auto var : lvl_master.undo_upper_) {
            lvl.undo_upper_.emplace_back(var);
        }
        solver.udiff_ = master.udiff_;
        solver.in_udiff_ = master.in_udiff_;

        // copy inactive
        lvl.inactive_.clear();
        for (auto *cs : lvl_master.inactive_) {
            lvl.inactive_.emplace_back(&solver.constraint_state(cs->constraint()));
        }

        // copy watches
        solver.var_watches_ = master.var_watches_;
        for (auto &var_watches : solver.var_watches_) {
            for (auto &watch : var_watches) {
                watch.second = &solver.constraint_state(watch.second->constraint());
            }
        }

        // copy removed watches
        lvl.removed_var_watches_.clear();
        lvl.removed_var_watches_.reserve(lvl_master.removed_var_watches_.size());
        for (auto const &[var, val, cs] : lvl_master.removed_var_watches_) {
            lvl.removed_var_watches_.emplace_back(var, val, &solver.constraint_state(cs->constraint()));
        }

        // copy todo queue
        solver.todo_.clear();
        solver.todo_.reserve(master.todo_.size());
        for (auto const &cs : master.todo_) {
            solver.todo_.emplace_back(&solver.constraint_state(cs->constraint()));
        }
    }

private:
    //! The associated decision level.
    level_t level_;
    //! Set of Clingcon::VarState objects with a modified lower bound.
    std::vector<var_t> undo_lower_;
    //! Set of Clingcon::VarState objects that a modified upper bound.
    std::vector<var_t> undo_upper_;
    //! List of constraint states that can become inactive on the next level.
    std::vector<AbstractConstraintState*> inactive_;
    //! List of variable/coefficient/constraint triples that have been removed
    //! from the Solver::v2cs_ map.
    std::vector<std::tuple<var_t, val_t, AbstractConstraintState*>> removed_var_watches_;
};

Solver::Solver(SolverConfig const &config, SolverStatistics &stats)
: config_{config}
, stats_{stats} {
    levels_.emplace_back(0);
}

Solver::Solver(Solver &&x) noexcept = default;

Solver::~Solver() = default;

void Solver::shrink_to_fit() {
    var2vs_.shrink_to_fit();
    litmap_.rehash(0);
    factmap_.shrink_to_fit();
    c2cs_.rehash(0);
    for (auto &watches : var_watches_) {
        watches.shrink_to_fit();
    }
    var_watches_.shrink_to_fit();
    udiff_.shrink_to_fit();
    ldiff_.shrink_to_fit();
    lit2cs_.rehash(0);
}

void Solver::copy_state(Solver const &master) {
    // just to be thorough
    split_last_ = master.split_last_;
    trail_offset_ = master.trail_offset_;
    minimize_level_ = master.minimize_level_;
    minimize_bound_ = master.minimize_bound_;

    // copy var states and lookups
    var2vs_ = master.var2vs_;
    factmap_ = master.factmap_;
    litmap_ = master.litmap_;

    // copy constraint states and lookups
    c2cs_.clear();
    lit2cs_.clear();
    for (auto const &[c, cs] : master.c2cs_) {
        auto ret = c2cs_.emplace(c, cs->copy());
        lit2cs_.emplace(c->literal(), ret.first->second.get());
    }

    // adjust levels
    Level::copy_state(*this, master);
}

var_t Solver::add_variable(val_t min_int, val_t max_int) {
    var_t idx = var2vs_.size();
    var2vs_.emplace_back(idx, min_int, max_int);
    var_watches_.emplace_back();
    ldiff_.emplace_back(0);
    udiff_.emplace_back(0);
    return idx;
}

std::optional<sum_t> Solver::minimize_bound() const {
    return minimize_bound_;
}

void Solver::update_minimize(AbstractConstraint &constraint, level_t level, sum_t bound) {
    if (!minimize_bound_.has_value() || bound < *minimize_bound_) {
        minimize_bound_ = bound;
        minimize_level_ = level;
        Level::mark_todo(*this, constraint_state(constraint));
    }
    else if (level < minimize_level_) {
        minimize_level_ = level;
        Level::mark_todo(*this, constraint_state(constraint));
    }
}

val_t Solver::get_value(var_t var) const {
    return var2vs_[var].lower_bound();
}

Solver::Level &Solver::level_() {
    return levels_.back();
}

void Solver::push_level_(level_t level) {
    assert(!levels_.empty());
    if (levels_.back().level() < level) {
        levels_.emplace_back(level);
    }
}

lit_t Solver::get_literal(AbstractClauseCreator &cc, VarState &vs, val_t value) {
    if (value < vs.min_bound()) {
        return -TRUE_LIT;
    }
    if (value >= vs.max_bound()) {
        return TRUE_LIT;
    }
    auto &lit = vs.get_or_add_literal(value);
    if (lit == 0) {
        lit = cc.add_literal();
        // Note: By default clasp's heuristic makes literals false. By flipping
        // the literal for non-negative values, assignments close to zero are
        // preferred. This way, we might get solutions with small numbers
        // first.
        if (value >= 0) {
            lit = -lit;
        }
        litmap_.emplace(lit, std::pair(vs.var(), value));
        cc.add_watch(lit);
        cc.add_watch(-lit);
    }
    return lit;
}

lit_t Solver::update_literal(AbstractClauseCreator &cc, VarState &vs, val_t value, Clingo::TruthValue truth) {
    // order literals can only be update on level 0
    if (truth == Clingo::TruthValue::Free || cc.assignment().decision_level() > 0) {
        return get_literal(cc, vs, value);
    }
    // the value is out of bounds
    if (value < vs.min_bound()) {
        return -TRUE_LIT;
    }
    if (value >= vs.max_bound()) {
        return TRUE_LIT;
    }
    auto &old = vs.get_or_add_literal(value);
    // there was no literal yet
    if (old == 0) {
        old = truth == Clingo::TruthValue::True ? TRUE_LIT : -TRUE_LIT;
        factmap_.emplace_back(old, vs.var(), value);
    }
    // we keep the literal
    return old;
}

void Solver::add_var_watch(var_t var, val_t i, AbstractConstraintState &cs) {
    assert(var < var_watches_.size());
    var_watches_[var].emplace_back(i, &cs);
}

void Solver::remove_var_watch(var_t var, val_t i, AbstractConstraintState &cs) {
    assert(var < var_watches_.size());
    auto &watches = var_watches_[var];
    watches.erase(std::find(watches.begin(), watches.end(), std::pair(i, &cs)));
}

void Solver::mark_inactive(AbstractConstraintState &cs) {
    level_().mark_inactive(cs);
}

AbstractConstraintState &Solver::add_constraint(AbstractConstraint &constraint) {
    auto &cs = c2cs_.emplace(&constraint, std::unique_ptr<AbstractConstraintState>{nullptr}).first->second;

    if (cs == nullptr) {
        cs = constraint.create_state();
        lit2cs_.emplace(constraint.literal(), cs.get());
        cs->attach(*this);
        Level::mark_todo(*this, *cs);
    }

    return *cs;
}

void Solver::remove_constraint(AbstractConstraint &constraint) {
    auto it = c2cs_.find(&constraint);
    auto &cs = *it->second;
    cs.detach(*this);

    for (auto rng = lit2cs_.equal_range(constraint.literal()); rng.first != rng.second; ++rng.first) {
        if (rng.first->second == &cs) {
            lit2cs_.erase(rng.first);
            break;
        }
    }

    level_().remove_constraint(*this, cs);
    c2cs_.erase(it);
}

bool Solver::translate(InitClauseCreator &cc, Statistics &stats, Config const &conf, ConstraintVec &constraints) {
    size_t jdx = 0, kdx = constraints.size(); // NOLINT
    for (size_t idx = jdx; idx < constraints.size(); ++idx) {
        auto &cs = add_constraint(*constraints[idx]);
        if (idx >= kdx) {
            ++stats.num_constraints;
            ++stats.translate_added;
        }
        auto ret = cs.translate(conf, *this, cc, constraints);
        if (!ret.first) {
            return false;
        }
        if (ret.second) {
            --stats.num_constraints;
            ++stats.translate_removed;
        }
        else {
            if (idx != jdx) {
                std::swap(constraints[idx], constraints[jdx]);
            }
            ++jdx;
        }
    }

    // Note: Constraints are removed by traversing the whole lookup table to
    // avoid potentially quadratic overhead if a large number of constraints
    // has to be removed.
    if (jdx < constraints.size()) {
        std::sort(constraints.begin() + jdx, constraints.end());
        auto in_removed = [jdx, &constraints] (AbstractConstraintState &cs) {
            struct {
                bool operator()(UniqueConstraint const &a, AbstractConstraint const *b) {
                    return a.get() < b;
                }
                bool operator()(AbstractConstraint const *b, UniqueConstraint const &a) {
                    return b < a.get();
                }
            } pred;
            return std::binary_search(constraints.begin() + jdx, constraints.end(), &cs.constraint(), pred);
        };

        level_().remove_constraints(*this, in_removed);

        for (auto it = lit2cs_.begin(); it != lit2cs_.end(); ) {
            if (in_removed(*it->second)) {
                it = lit2cs_.erase(it);
            }
            else {
                ++it;
            }
        }
        for (auto it = constraints.begin() + jdx, ie = constraints.end(); it != ie; ++it) {
            c2cs_.erase(it->get());
        }
    }

    constraints.erase(constraints.begin() + jdx, constraints.end());

    return true;
}

bool Solver::simplify(AbstractClauseCreator &cc, bool check_state) {
    auto ass = cc.assignment();
    auto trail = ass.trail();

    // Note: The initial propagation below, will not introduce any order
    // literals other than true or false.
    while (true) {
        if (!cc.propagate()) {
            return false;
        }

        auto trail_offset = trail.size();
        if (trail_offset_ == trail_offset && todo_.empty()) {
            return true;
        }

        if (!propagate_(cc, trail.begin() + trail_offset_, trail.begin() + trail_offset)) {
            return false;
        }
        trail_offset_ = trail_offset;

        if (!check(cc, check_state)) {
            return false;
        }
    }
}

[[nodiscard]] bool Solver::propagate(AbstractClauseCreator &cc, Clingo::LiteralSpan changes) {
    return propagate_(cc, changes.begin(), changes.end());
}

template <class It>
[[nodiscard]] bool Solver::propagate_(AbstractClauseCreator &cc, It begin, It end) {
    Timer timer{stats_.time_propagate};

    auto ass = cc.assignment();

    // open a new decision level if necessary
    push_level_(ass.decision_level());

    // propagate order literals that became true/false
    for (auto it = begin; it != end; ++it) {
        if (!propagate_(cc, *it)) {
            return false;
        }
    }

    return true;
}

bool Solver::propagate_(AbstractClauseCreator &cc, lit_t lit) {
    for (auto rng = lit2cs_.equal_range(lit); rng.first != rng.second; ++rng.first) {
        Level::mark_todo(*this, *rng.first->second);
    }
    return update_domain_(cc, lit);
}

template <int sign, class It>
bool Solver::propagate_variables_(AbstractClauseCreator &cc, lit_t reason_lit, It begin, It end) {
    auto ass = cc.assignment();

    for (auto it = begin; it != end; ++it) {
        auto lit = sign * it->second;
        if (ass.is_true(lit)) {
            break;
        }
        if (!cc.add_clause({-reason_lit, lit}, reason_lit != TRUE_LIT ? Clingo::ClauseType::Learnt : Clingo::ClauseType::Static)) {
            return false;
        }
        // Note: Literal reason_lit is already guaranteed to be a fact on level 0.
        if (config_.propagate_chain && ass.decision_level() > 0) {
            reason_lit = lit;
        }
    }

    return true;
}

bool Solver::update_upper_(Level &lvl, AbstractClauseCreator &cc, var_t var, lit_t lit, val_t value) {
    auto &vs = var_state(var);
    if (vs.upper_bound() > value) {
        lvl.update_upper(*this, vs, value);
    }
    return propagate_variables_<1>(cc, lit, vs.lit_gt(value), vs.end());
}

bool Solver::update_lower_(Level &lvl, AbstractClauseCreator &cc, var_t var, lit_t lit, val_t value) {
    auto &vs = var_state(var);
    if (vs.lower_bound() < value + 1) {
        lvl.update_lower(*this, vs, value);
    }
    return propagate_variables_<-1>(cc, lit, vs.lit_lt(value), vs.rend());
}

bool Solver::update_domain_(AbstractClauseCreator &cc, lit_t lit) {
    auto &lvl = level_();
    auto ass = cc.assignment();
    assert(ass.is_true(lit));

    // On-the-fly simplification on the top-level.
    if (lit != TRUE_LIT && ass.decision_level() == 0 && ass.is_fixed(lit)) {
        for (auto rng = litmap_.equal_range(lit); rng.first != rng.second; ++rng.first) {
            auto [var, value] = rng.first->second;
            factmap_.emplace_back(TRUE_LIT, var, value);
        }
        for (auto rng = litmap_.equal_range(-lit); rng.first != rng.second; ++rng.first) {
            auto [var, value] = rng.first->second;
            factmap_.emplace_back(-TRUE_LIT, var, value);
        }
        lit = TRUE_LIT;
    }

    // Note: Neither factmap_ nor litmap_ will be modified by update_lower_ /
    // update_upper_.

    // Fact propagation. (Could also be put in the litmap...)
    if (lit == TRUE_LIT) {
        assert(ass.decision_level() == 0);
        for (auto [fact_lit, var, value] : factmap_) {
            auto &vs = var_state(var);
            if (fact_lit == TRUE_LIT) {
                if (!update_upper_(lvl, cc, var, lit, value)) {
                    return false;
                }
                vs.unset_literal(value);
            }
            else {
                if (!update_lower_(lvl, cc, var, lit, value)) {
                    return false;
                }
                vs.unset_literal(value - 1);
            }
        }
        factmap_.clear();
        return true;
    }

    // Note: Note that iterators of unordered_multimap's are guaranteed to stay
    // valid even for insertion.
    for (auto rng = litmap_.equal_range(lit); rng.first != rng.second; ++rng.first) {
        auto [var, value] = rng.first->second;
        if (!update_upper_(lvl, cc, var, lit, value)) {
            return false;
        }
    }

    for (auto rng = litmap_.equal_range(-lit); rng.first != rng.second; ++rng.first) {
        auto [var, value] = rng.first->second;
        if (!update_lower_(lvl, cc, var, lit, value)) {
            return false;
        }
    }

    return true;
}

bool Solver::check(AbstractClauseCreator &cc, bool check_state) {
    Timer timer(stats_.time_check);

    auto ass = cc.assignment();
    auto &lvl = level_();

    // Note: Most of the time check has to be called only for levels that have
    // also been propagated. The exception is if a minimize constraint has to
    // be integrated when backtracking from a bound update.
    if (ass.decision_level() != lvl.level() && lvl.level() >= minimize_level_) {
        return true;
    }

    // Note: We have to loop here because watches for the true/false literals
    // do not fire again.
    while (true) {
        // Note: This integrates any facts that have not been integrated yet on
        // the top level.
        if (!factmap_.empty()) {
            assert(ass.decision_level() == 0);
            if (!update_domain_(cc, TRUE_LIT)) {
                return false;
            }
        }

        // update the bounds of the constraints (this is the only place where
        // the todo queue is filled after initializaton)
        for (auto var : in_udiff_) {
            lvl.update_constraints_(*this, var, udiff_[var]);
            udiff_[var] = 0;
        }
        in_udiff_.clear();
        for (auto var : in_ldiff_) {
            lvl.update_constraints_(*this, var, ldiff_[var]);
            ldiff_[var] = 0;
        }
        in_ldiff_.clear();

        // propagate affected constraints
        bool ret{true};
        for (auto *cs : todo_) {
            cs->mark_todo(false);
            if (!ret) {
                continue;
            }

            if (!ass.is_false(cs->constraint().literal())) {
                if (!cs->propagate(*this, cc, check_state)) {
                    ret = false;
                }
            }
            else {
                lvl.mark_inactive(*cs);
            }
        }
        todo_.clear();

        if (!ret || factmap_.empty()) {
            return ret;
        }
    }
}

void Solver::undo() {
    Timer timer{stats_.time_undo};

    auto &lvl = level_();

    lvl.undo(*this);

    levels_.pop_back();
}

void Solver::check_full(AbstractClauseCreator &cc, bool check_solution) {
    auto split = [&](VarState &vs) {
        if (!vs.is_assigned()) {
            auto value = midpoint(vs.lower_bound(), vs.upper_bound());
            static_cast<void>(get_literal(cc, vs, value));
            return true;
        }
        return false;
    };

    if (config_.split_all) {
        bool res{false};
        for (auto &vs : var2vs_) {
            res = split(vs) || res;
        }
        if (res) {
            return;
        }
    }
    else {
        auto ib = var2vs_.begin();
        auto im = ib + split_last_;
        auto ie = var2vs_.end();

        auto split_once = [&](auto it) {
            if (split(*it)) {
                split_last_ = it - ib;
                return true;;
            }
            return false;
        };

        for (auto it = im; it != ie; ++it) {
            if (split_once(it)) {
                return;
            }
        }
        for (auto it = ib; it != im; ++it) {
            if (split_once(it)) {
                return;
            }
        }
    }

    if (check_solution) {
        auto ass = cc.assignment();
        for (auto [lit, cs] : lit2cs_) {
            if (ass.is_true(lit)) {
                cs->check_full(*this);
            }

        }
    }
}

void Solver::update(AbstractClauseCreator &cc) {
    auto ass = cc.assignment();

    // reset minimize state
    minimize_bound_.reset();
    minimize_level_ = 0;

    // remove solve step local variables from litmap_
    for (auto it = litmap_.begin(), ie = litmap_.end(); it != ie; ) {
        if (!ass.has_literal(it->first)) {
            auto &vs = var_state(it->second.first);
            vs.unset_literal(it->second.second);
            it = litmap_.erase(it);
        }
        else if (auto truth = ass.truth_value(it->first); truth != Clingo::TruthValue::Free) {
            // Note: It might also be possible to just unset factual literals.
            // Adding them to the factmap_ should definitely work because check
            // will clear its values later.
            auto &vs = var_state(it->second.first);
            auto lit = ass.is_true(it->first) ? TRUE_LIT : -TRUE_LIT;
            factmap_.emplace_back(lit, it->second.first, it->second.second);
            vs.set_literal(it->second.second, lit);
            it = litmap_.erase(it);
        }
        else {
            ++it;
        }
    }

    // we sort here because the above iteration order is unspecified
    std::sort(factmap_.begin(), factmap_.end());
}

bool Solver::cleanup_literals(AbstractClauseCreator &cc, bool check_state) {
    // make sure that all top level literals are mapped to the fact literal
    update(cc);

    // update_domain_ in check makes sure that unnecassary facts are removed
    return check(cc, check_state);
}

bool Solver::update_bounds(AbstractClauseCreator &cc, Solver &other, bool check_state) {
    auto it = var2vs_.begin();
    for (auto &vs_other : other.var2vs_) {
        auto &vs = *it++;

        // update upper bounds
        if (vs_other.upper_bound() < vs.upper_bound()) {
            auto lit = update_literal(cc, vs, vs_other.upper_bound(), Clingo::TruthValue::True);
            if (!cc.add_clause({lit})) {
                return false;
            }
        }

        // update lower bounds
        if (vs.lower_bound() < vs_other.lower_bound()) {
            auto lit = update_literal(cc, vs, vs_other.lower_bound()-1, Clingo::TruthValue::False);
            if (!cc.add_clause({-lit})) {
                return false;
            }
        }
    }

    // update_domain_ in check makes sure that unnecassary facts are removed
    return check(cc, check_state);
}

bool Solver::add_dom(AbstractClauseCreator &cc, lit_t lit, var_t var, IntervalSet<val_t> const &domain) {
    auto ass = cc.assignment();
    if (ass.is_false(lit)) {
        return true;
    }
    if (ass.is_true(lit)) {
        lit = TRUE_LIT;
    }
    auto &vs = var_state(var);

    std::optional<val_t> py;
    for (auto [x, y] : domain) {
        auto ly = py.has_value() ? -get_literal(cc, vs, *py - 1) : TRUE_LIT;
        auto truth = Clingo::TruthValue::Free;
        if (lit == TRUE_LIT && ass.is_true(ly)) {
            truth = Clingo::TruthValue::False;
        }
        auto lx = update_literal(cc, vs, x-1, truth);
        if (!cc.add_clause({-lit, -ly, -lx})) {
            return false;
        }
        py = y;
    }

    std::optional<val_t> px;
    for (auto it = domain.rbegin(), ie = domain.rend(); it != ie; ++it) {
        auto [x, y] = *it;
        auto lx = px.has_value() ? get_literal(cc, vs, *px - 1) : TRUE_LIT;
        auto truth = Clingo::TruthValue::Free;
        if (lit == TRUE_LIT && ass.is_true(lx)) {
            truth = Clingo::TruthValue::True;
        }
        auto ly = update_literal(cc, vs, y-1, truth);
        if (!cc.add_clause({-lit, -lx, ly})) {
            return false;
        }
        px = x;
    }

    return true;
}

bool Solver::add_simple(AbstractClauseCreator &cc, lit_t clit, val_t co, var_t var, val_t rhs, bool strict) {
    auto ass = cc.assignment();

    // the constraint is never propagated
    if (!strict && ass.is_false(clit)) {
        return true;
    }

    auto &vs = var_state(var);

    Clingo::TruthValue truth;
    val_t value{0};
    if (co > 0) {
        truth = ass.truth_value(clit);
        value = floordiv(rhs, co);
    }
    else {
        truth = ass.truth_value(-clit);
        value = -floordiv(rhs, -co) - 1;
    }

    // in this case we can use the literal of the constraint as order variable
    if (strict && vs.min_bound() <= value && value < vs.max_bound() && !vs.has_literal(value)) {
        auto lit = clit;
        if (co < 0) {
            lit = -lit;
        }
        if (truth == Clingo::TruthValue::Free) {
            cc.add_watch(lit);
            cc.add_watch(-lit);
            litmap_.emplace(lit, std::pair(vs.var(), value));
        }
        else {
            lit = truth == Clingo::TruthValue::True ? TRUE_LIT : -TRUE_LIT;;
            factmap_.emplace_back(lit, vs.var(), value);
        }
        vs.set_literal(value, lit);
    }
    // otherwise we just update the existing order literal
    else {
        auto lit = update_literal(cc, vs, value, truth);
        if (co < 0) {
            lit = -lit;
        }
        if (!cc.add_clause({-clit, lit})) {
            return false;
        }
        if (strict && !cc.add_clause({-lit, clit})) {
            return false;
        }
    }

    return true;
}

} // namespace Clingcon
