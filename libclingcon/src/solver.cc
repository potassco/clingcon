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
        if (level_ > 0 && vs.pushed_lower(level_) ) {
            vs.push_lower(level_);
            undo_lower_.emplace_back(vs.var());
        }
        vs.lower_bound(value + 1);

        // TODO: ldiff is not initialized yet
        if (solver.ldiff_[vs.var()] == 0) {
            solver.in_ldiff_.emplace_back(vs.var());
        }
        solver.ldiff_[vs.var()] += diff;
    }

    //! Update the upper bound of a var state.
    void update_upper(Solver &solver, VarState &vs, val_t value) {
        val_t diff = value - vs.upper_bound();

        if (level_ > 0 && vs.pushed_upper(level_) ) {
            vs.push_upper(level_);
            undo_upper_.emplace_back(vs.var());
        }
        vs.upper_bound(value);

        // TODO: udiff is not initialized yet
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
            if (value_cs.second->removable(level_)) {
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
        if (cs.mark_todo(true)) {
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
            auto vs = solver.var_state(var);
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
            auto vs = solver.var_state(var);
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

Solver::~Solver() = default;


void Solver::copy_state(Solver const &master) {
    // just to be thorough
    split_last_ = master.split_last_;
    trail_offset_ = master.trail_offset_;
    minimize_level_ = master.minimize_level_;
    minimize_bound_ = master.minimize_bound_;

    // make sure we have an empty var state for each variable
    var_t var = 0;
    var2vs_.reserve(master.var2vs_.size());
    for (auto const &vs_master : master.var2vs_) {
        if (var2vs_.size() <= var) {
            static_cast<void>(add_variable(vs_master.min_bound(), vs_master.max_bound()));
        }
        else {
            auto &vs = var2vs_[var];
            vs.reset(vs_master.min_bound(), vs_master.max_bound());
        }
        ++var;
    }

    // copy the fact map
    factmap_ = master.factmap_;
    for (auto const &[lit, var, val] : factmap_) {
        var_state(var).set_literal(val, lit);
    }

    // copy the order literal map
    litmap_ = master.litmap_;
    for (auto const &[lit, var_val] : litmap_) {
        var_state(var_val.first).set_literal(var_val.second, lit);
    }

    // copy the map from literals to var states
    for (auto const &[c, cs] : master.c2cs_) {
        c2cs_.emplace(c, cs->copy());
    }

    // adjust levels
    Level::copy_state(*this, master);
}

var_t Solver::add_variable(val_t min_int, val_t max_int) {
    var_t idx = var2vs_.size();
    var2vs_.emplace_back(idx, min_int, max_int);
    return idx;
}

std::optional<val_t> Solver::minimize_bound() const {
    return minimize_bound_;
}

void Solver::update_minimize(AbstractConstraint &constraint, level_t level, val_t bound) {
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

void Solver::remove_literal_(var_t var, lit_t lit, val_t value) {
    assert(lit != TRUE_LIT && lit != -TRUE_LIT);

    for (auto rng = litmap_.equal_range(lit); rng.first != rng.second; ++rng.first) {
        if (rng.first->second.first == var && rng.first->second.second == value) {
            litmap_.erase(rng.first);
            return;
        }
    }

    assert(false && "could not remove literal");
}

std::pair<bool, lit_t> Solver::update_literal(AbstractClauseCreator &cc, VarState &vs, val_t value, std::optional<bool> truth) {
    // order literals can only be update on level 0
    if (!truth.has_value() || cc.assignment().decision_level() > 0) {
        return {true, get_literal(cc, vs, value)};
    }
    auto lit = *truth ? TRUE_LIT : -TRUE_LIT;
    auto ret = true;
    // the value is out of bounds
    if (value < vs.min_bound()) {
        auto old = -TRUE_LIT;
        if (old != lit) {
             ret = cc.add_clause({*truth ? old : -old});
        }
    }
    else if (value >= vs.max_bound()) {
        auto old = TRUE_LIT;
        if (old != lit) {
             ret = cc.add_clause({*truth ? old : -old});
        }
    }
    // there was no literal yet
    else if (auto &old = vs.get_or_add_literal(value); old == 0) {
        old = lit;
        factmap_.emplace_back(lit, vs.var(), value);
    }
    // the old literal has to be replaced
    else if (old != lit) {
        old = lit;
        remove_literal_(vs.var(), old, value);
        factmap_.emplace_back(lit, vs.var(), value);
        ret = cc.add_clause({*truth ? old : -old});
    }
    return {ret, lit};
}

void Solver::add_var_watch(var_t var, val_t i, AbstractConstraintState *cs) {
    // TODO: var_watches_ is not resized yet
    assert(var < var_watches_.size());
    var_watches_[var].emplace_back(i, cs);
}

void Solver::remove_var_watch(var_t var, val_t i, AbstractConstraintState *cs) {
    // TODO: var_watches_ is not resized yet
    assert(var < var_watches_.size());
    auto &watches = var_watches_[var];
    watches.erase(std::find(watches.begin(), watches.end(), std::pair(i, cs)));
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

    for (auto rng = lit2cs_.equal_range(constraint.literal()); rng.first != rng.second; ) {
        if (rng.first->second == &cs) {
            lit2cs_.erase(rng.first);
        }
        else {
            ++rng.first;
        }
    }

    level_().remove_constraint(*this, cs);
    c2cs_.erase(it);
}

bool Solver::translate(AbstractClauseCreator &cc, Statistics &stats, Config const &conf, ConstraintVec &constraints) {
    std::vector<AbstractConstraintState*> removed;

    size_t jdx = 0, kdx = constraints.size(); // NOLINT
    for (size_t idx = jdx; idx < constraints.size(); ++idx) {
        auto &cs = add_constraint(*constraints[idx]);
        if (idx >= kdx) {
            ++stats.num_constraints;
            ++stats.translate_added;
        }
        auto ret = cs.translate(cc, conf, constraints);
        if (!ret.first) {
            return false;
        }
        if (ret.second) {
            --stats.num_constraints;
            ++stats.translate_removed;
            removed.emplace_back(&cs);
            if (idx != jdx) {
                std::swap(constraints[idx], constraints[jdx]);
            }
            ++jdx;
        }
    }

    // Note: Constraints are removed by traversing the whole lookup table to
    // avoid potentially quadratic overhead if a large number of constraints
    // has to be removed.
    if (removed.empty()) {
        std::sort(removed.begin(), removed.end());
        auto in_removed = [&removed] (AbstractConstraintState &cs) {
            return std::binary_search(removed.begin(), removed.end(), &cs);

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

        for (auto *cs : removed) {
            c2cs_.erase(&cs->constraint());
        }
    }

    // Note: even though it is not strictly necessary this keeps the
    // constraints valid until all constraint states have been removed
    constraints.resize(jdx);

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
    for (auto &[lit, cs] : lit2cs_) {
        Level::mark_todo(*this, *cs);
    }
    return update_domain_(cc, lit);
}

template <int sign>
bool Solver::propagate_variable_(AbstractClauseCreator &cc, VarState &vs, val_t value, lit_t lit) {
    auto ass = cc.assignment();
    assert(ass.is_true(lit));
    assert(vs.has_literal(value));

    // get the literal to propagate
    // Note: this explicetly does not use get_literal
    auto con = sign * *vs.get_literal(value);

    // on-the-fly simplify
    if (ass.is_fixed(lit) && !ass.is_fixed(con)) {
        auto [ret, con] = update_literal(cc, vs, value, sign > 0);
        if (!ret) {
            return false;
        }
        con = sign*con;
    }

    // propagate the literal
    if (!ass.is_true(con)) {
        if (!cc.add_clause({-lit, con})) {
            return false;
        }
    }

    return true;
}

template <int sign, class It>
bool Solver::propagate_variables_(AbstractClauseCreator &cc, VarState &vs, lit_t reason_lit, It begin, It end) {
    auto ass = cc.assignment();

    for (auto it = begin; it != end; ++it) {
        auto [value, lit] = *it;
        if (ass.is_true(sign * lit)) {
            break;
        }
        if (!propagate_variable_<sign>(cc, vs, value, reason_lit)) {
            return false;
        }
        // Note: Literals might be uppdated on level 0 and the reason_lit is
        // already guaranteed to be a fact on level 0.
        if (config_.propagate_chain && ass.decision_level() > 0) {
            reason_lit = sign * lit;
        }
    }

    return true;
}

bool Solver::update_upper_(Level &lvl, AbstractClauseCreator &cc, var_t var, lit_t lit, val_t value) {
    auto vs = var_state(var);
    if (vs.upper_bound() > value) {
        lvl.update_upper(*this, vs, value);
    }
    return propagate_variables_<1>(cc, vs, lit, vs.lit_gt(value), vs.end());
}

bool Solver::update_lower_(Level &lvl, AbstractClauseCreator &cc, var_t var, lit_t lit, val_t value) {
    auto vs = var_state(var);
    if (vs.lower_bound() < value + 1) {
        lvl.update_lower(*this, vs, value);
    }
    return propagate_variables_<-1>(cc, vs, lit, vs.lit_lt(value), vs.rend());
}

bool Solver::update_domain_(AbstractClauseCreator &cc, lit_t lit) {
    auto &lvl = level_();

    assert(lit != -TRUE_LIT);
    if (lit == TRUE_LIT) {
        for (auto [fact_lit, var, value] : factmap_) {
            auto vs = var_state(var);
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
                if (!cs->propagate(cc, config_, check_state)) {
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

/*
class State(object):
    def update(self, cc):
        """
        This function resets a state and should be called when a new solve step
        is started.

        This function removes all solve step local variables from the state,
        maps fixed global literals to the true/false literal, and resets the
        minimize constraint.
        """
        ass = cc.assignment

        self._minimize_bound = None
        self._minimize_level = 0

        remove_invalid = []
        remove_fixed = []
        for lit, vss in self.litmap_.items():
            if abs(lit) == TRUE_LIT:
                continue

            if not ass.has_literal(lit):
                remove_invalid.append((lit, vss))
            elif ass.is_fixed(lit):
                remove_fixed.append((lit, vss))

        # remove solve step local variables
        # Note: Iteration order does not matter.
        for lit, vss in remove_invalid:
            for vs, value in vss:
                vs.unset_literal(value)
            del self.litmap_[lit]

        # Note: Map bounds associated with top level facts to true/false.
        # Because we do not know if the facts have already been propagated, we
        # simply append them and do not touch the counts for integrated facts.
        for old, vss in sorted(remove_fixed):
            for vs, value in vss:
                lit = TRUE_LIT if ass.is_true(old) else -TRUE_LIT
                self.litmap_.setdefault(lit, []).append((vs, value))
                vs.set_literal(value, lit)
            del self.litmap_[old]

    def _cleanup_literals(self, cc, lit, pred):
        """
        Remove (var,value) pairs associated with `lit` that match `pred`.
        """
        assert lit in (TRUE_LIT, -TRUE_LIT)
        if lit in self.litmap_:
            variables = self.litmap_[lit]

            # adjust the number of facts that have been integrated
            idx = 0 if lit == TRUE_LIT else 1
            nums = list(self.facts_integrated_)
            for x in variables[:nums[idx]]:
                if pred(x):
                    nums[idx] -= 1
            self.facts_integrated_ = tuple(nums)

            # remove values matching pred
            i = remove_if(variables, pred)
            assert i > 0
            for vs, value in variables[i:]:
                old = vs.get_literal(value)
                if old != lit:
                    # Note: This case cannot be triggered if propagation works
                    # correctly because facts can only be propagated on level
                    # 0. But to be on the safe side in view of theory
                    # extensions, this makes the old literal equal to lit
                    # before removing the old literal.
                    if not cc.add_clause([-lit, old], lock=True):
                        return False
                    if not cc.add_clause([-old, lit], lock=True):
                        return False
                    self._remove_literal(vs, old, value)
                vs.unset_literal(value)
            del variables[i:]

        return True

    def cleanup_literals(self, cc):
        """
        Remove all order literals associated with facts that are above the
        upper or below the lower bound.
        """
        # make sure that all top level literals are assigned to the fact literal
        self.update(cc)

        # cleanup
        return (self._cleanup_literals(cc, TRUE_LIT, lambda x: x[1] != x[0].upper_bound) and
                self._cleanup_literals(cc, -TRUE_LIT, lambda x: x[1] != x[0].lower_bound-1))

    def update_bounds(self, cc, other):
        """
        Integrate the lower and upper bounds from State `other`.

        The function might add clauses via `cc` to fix literals that have to be
        updated. This can lead to a conflict if states have conflicting
        lower/upper bounds.

        Precondition: update should be called before this function to really
                      integrate all bounds.
        """
        # pylint: disable=protected-access

        # update upper bounds
        for vs_b, _ in other.litmap_.get(TRUE_LIT, []):
            vs_a = self.var2vs_[vs_b.var]
            if vs_b.upper_bound < vs_a.upper_bound:
                ret, _ = self.update_literal(vs_a, vs_b.upper_bound, cc, True)
                if not ret:
                    return False

        # update lower bounds
        for vs_b, _ in other.litmap_.get(-TRUE_LIT, []):
            vs_a = self.var2vs_[vs_b.var]
            if vs_a.lower_bound < vs_b.lower_bound:
                ret, _ = self.update_literal(vs_a, vs_b.lower_bound-1, cc, False)
                if not ret:
                    return False

        return self._update_domain(cc, 1)

    def add_dom(self, cc, literal, var, domain):
        """
        Integrates the given domain for varibale var.

        Consider x in {[1,3), [4,6), [7,9)}. We can simply add the binary
        constraints:
        - right to left
          - true => x < 9
          - x < 7 => x < 6
          - x < 4 => x < 3
        - left to right
          - true => x >= 1
          - x >= 3 => x >= 4
          - x >= 6 => x >= 7
        """
        ass = cc.assignment
        if ass.is_false(literal):
            return True
        if ass.is_true(literal):
            literal = TRUE_LIT
        vs = self.var_state(var)

        py = None
        for x, y in domain:
            ly = TRUE_LIT if py is None else -self.get_literal(vs, py-1, cc)
            true = literal == TRUE_LIT and ass.is_true(ly)
            ret, lx = self.update_literal(vs, x-1, cc, not true and None)
            if not ret or not cc.add_clause([-literal, -ly, -lx]):
                return False
            py = y

        px = None
        for x, y in reversed(domain):
            ly = TRUE_LIT if px is None else self.get_literal(vs, px-1, cc)
            true = literal == TRUE_LIT and ass.is_true(ly)
            ret, lx = self.update_literal(vs, y-1, cc, true or None)
            if not ret or not cc.add_clause([-literal, -ly, lx]):
                return False
            px = x

        return True

    def add_simple(self, cc, clit, co, var, rhs, strict):
        """
        This function integrates singleton constraints intwatches_o the state.

        We explicitely handle the strict case here to avoid introducing
        unnecessary literals.
        """
        # pylint: disable=protected-access

        ass = cc.assignment

        # the constraint is never propagated
        if not strict and ass.is_false(clit):
            return True

        vs = self.var_state(var)

        if co > 0:
            truth = ass.value(clit)
            value = rhs//co
        else:
            truth = ass.value(-clit)
            value = -(rhs//-co)-1

        # in this case we can use the literal of the constraint as order variable
        if strict and vs.min_bound <= value < vs.max_bound and not vs.has_literal(value):
            lit = clit
            if co < 0:
                lit = -lit
            if truth is None:
                cc.add_watch(lit)
                cc.add_watch(-lit)
            elif truth:
                lit = TRUE_LIT
            else:
                lit = -TRUE_LIT
            vs.set_literal(value, lit)
            self.litmap_.setdefault(lit, []).append((vs, value))

        # otherwise we just update the existing order literal
        else:
            ret, lit = self.update_literal(vs, value, cc, truth)
            if not ret:
                return False
            if co < 0:
                lit = -lit
            if not cc.add_clause([-clit, lit]):
                return False
            if strict and not cc.add_clause([-lit, clit]):
                return False

        return True
*/

} // namespace Clingcon
