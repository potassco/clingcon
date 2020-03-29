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
    void update_lower(Solver &solver, var_t var, val_t value) {
        auto &vs = solver.var_state(var);
        val_t diff = value + 1 - vs.lower_bound();
        if (level_ > 0 && vs.pushed_lower(level_) ) {
            vs.push_lower(level_);
            undo_lower_.emplace_back(var);
        }
        vs.lower_bound(value + 1);

        if (solver.ldiff_[vs.var()] == 0) {
            solver.in_ldiff_.emplace_back(var);
        }
        solver.ldiff_[var] += diff;
    }

    //! Update the upper bound of a var state.
    void update_upper(Solver &solver, var_t var, val_t value) {
        auto &vs = solver.var_state(var);
        val_t diff = value - vs.upper_bound();
        if (level_ > 0 && vs.pushed_upper(level_) ) {
            vs.push_upper(level_);
            undo_upper_.emplace_back(var);
        }
        vs.upper_bound(value);

        if (solver.udiff_[var] == 0) {
            solver.in_udiff_.emplace_back(var);
        }
        solver.udiff_[var] += diff;
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
        for (auto &[var, val, cs] : removed_watches_) {
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
            solver.todo_.erase(std::find(solver.todo_.begin(), solver.todo_.end(), &cs));

        }
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

        lvl.inactive_.clear();
        for (auto *cs : lvl_master.inactive_) {
            lvl.inactive_.emplace_back(&solver.constraint_state(cs->constraint()));
        }

        lvl.removed_watches_.clear();
        lvl.removed_watches_.reserve(lvl_master.removed_watches_.size());
        for (auto const &[var, val, cs] : lvl_master.removed_watches_) {
            lvl.removed_watches_.emplace_back(var, val, &solver.constraint_state(cs->constraint()));
        }

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
    std::vector<std::tuple<var_t, val_t, AbstractConstraintState*>> removed_watches_;
};

Solver::Solver(SolverConfig const &config, SolverStatistics &stats)
: config_{config}
, stats_{stats} {
    levels_.emplace_back(0);
}

Solver::~Solver() = default;


void Solver::copy_state(Solver const &master) {
    // adjust integrated facts
    facts_integrated_ = master.facts_integrated_;

    // make sure we have an empty var state for each variable
    var_t var = 0;
    var2vs_.reserve(master.var2vs_.size());
    for (auto const &vs_master : master.var2vs_) {
        if (var2vs_.size() <= var) {
            add_variable(vs_master.min_bound(), vs_master.max_bound());
        }
        else {
            auto &vs = var2vs_[var];
            vs.reset(vs_master.min_bound(), vs_master.max_bound());
        }
        ++var;
    }

    // copy the map from literals to var states
    litmap_ = master.litmap_;
    for (auto const &[lit, var_val] : litmap_) {
        var_state(var_val.first).set_literal(var_val.second, lit);
    }

    // copy the map from literals to var states
    for (auto const &[c, cs] : master.c2cs_) {
        c2cs_.emplace(c, cs->copy());
    }

    // copy watches
    var_watches_ = master.var_watches_;
    for (auto &var_watches : var_watches_) {
        for (auto &watch : var_watches) {
            watch.second = &constraint_state(watch.second->constraint());
        }
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

Solver::Level &Solver::push_level_(level_t level) {
    assert(!levels_.empty());
    if (levels_.back().level() < level) {
        levels_.emplace_back(level);
    }
    return level_();
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
        litmap_.emplace(lit, std::pair(vs.var(), value));
    }
    // the old literal has to be replaced
    else if (old != lit) {
        old = lit;
        remove_literal_(vs.var(), old, value);
        litmap_.emplace(lit, std::pair(vs.var(), value));
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
        cs->attach(*this);
        Level::mark_todo(*this, *cs);
    }

    return *cs;
}

void Solver::remove_constraint(AbstractConstraint &constraint) {
    auto it = c2cs_.find(&constraint);
    auto &cs = *it->second;
    cs.detach(*this);
    level_().remove_constraint(*this, cs);
    c2cs_.erase(it);
}

/*
class State(object):
    def translate(self, cc, l2c, stats, config):
        """
        Translate constraints in the map l2c and return a list of constraint
        added during translation.

        This functions removes translated constraints from the map and the
        state. Constraints added during the translation have to be added to the
        propagator as well.
        """
        remove_cs = set()
        added = []

        def _translate(constraints, count):
            i = j = 0
            while i < len(constraints):
                i, cs = i+1, self.add_constraint(constraints[i])
                if count:
                    stats.num_constraints += 1
                    stats.translate_added += 1
                ret, rem = cs.translate(cc, self, config, added)
                if not ret:
                    return False
                if rem:
                    stats.num_constraints -= 1
                    stats.translate_removed += 1
                    remove_cs.add(cs)
                    continue
                if i-1 != j:
                    constraints[i-1], constraints[j] = constraints[j], constraints[i-1]
                j += 1
            del constraints[j:]

        for lit in sorted(l2c):
            _translate(l2c[lit], False)
        _translate(added, True)

        # Note: Constraints are removed by traversing the whole lookup table to
        # avoid potentially quadratic overhead if a large number of constraints
        # has to be removed.
        if remove_cs:
            remove_vars = []
            for var, css in self.var_watches_.items():
                i = remove_if(css, lambda cs: cs[1] in remove_cs)
                del css[i:]
                if not css:
                    remove_vars.append(var)
            for var in remove_vars:
                del self.var_watches_[var]

            # Note: In theory all inactive constraints should be remove on level 0.
            i = remove_if(self._level.inactive, lambda cs: cs in remove_cs)
            del self._level.inactive[i:]

            for cs in remove_cs:
                del self.c2cs_[cs.constraint]

            self._todo = TodoList(cs for cs in self._todo if cs not in remove_cs)

        return cc.commit(), added

    def simplify(self, cc, check_state):
        """
        Simplify the state using fixed literals in the trail up to the given
        offset and the enqued constraints in the todo list.

        Note that this functions assumes that newly added constraints have been
        enqueued before.
        """
        # Note: Propagation won't add anything to the trail because atm
        # there are no order literals which could be propagated. This
        # might change in the multi-shot case when order literals have
        # been added in a previous step which are then implied by the
        # newly added constraints.
        ass = cc.assignment
        trail = ass.trail

        # Note: The initial propagation below, will not introduce any order
        # literals other than true or false.
        while True:
            if not cc.propagate():
                return False

            trail_offset = len(trail)
            if self._trail_offset == trail_offset and not self._todo:
                return True

            if not self.propagate(cc, trail[self._trail_offset:trail_offset]):
                return False
            self._trail_offset = trail_offset

            if not self.check(cc, check_state):
                return False

    # propagation
    @measure_time_decorator("statistics.time_propagate")
    def propagate(self, cc, changes):
        """
        Propagates constraints and order literals.

        Constraints that became true are added to the todo list and bounds of
        variables are adjusted according to the truth of order literals.
        """
        # Note: This function has to be as fast as possible. In C++ we can try
        # to put all relevant data into the litmap to make the function as
        # cache-friendly as possible. Max also noted that it might help to
        # propagate all order literals affected by an assignment and not just
        # the neighboring one to avoid "rippling" propagate calls.
        ass = cc.assignment

        # open a new decision level if necessary
        self._push_level(ass.decision_level)

        # propagate order literals that became true/false
        for lit in changes:
            self._todo.extend(map(self.constraint_state, self._l2c.get(lit, [])))
            if not self._update_domain(cc, lit):
                return False

        return True

    def _propagate_variable(self, cc, vs, value, lit, sign):
        """
        Propagates the preceeding or succeeding order literal of lit.

        Whether the target literal is a preceeding or succeeding literal is
        determined by `sign`. The target order literal is given by
        `(vs.var,value)` and must exist.

        For example, if `sign==1`, then lit is an order literal for some
        integer value smaller than `value`. The function propagates the clause
        `lit` implies `vs.get_literal(value)`.

        Furthermore, if `lit` is a fact, the target literal is simplified to a
        fact, too.
        """

        ass = cc.assignment
        assert ass.is_true(lit)
        assert vs.has_literal(value)

        # get the literal to propagate
        # Note: this explicetly does not use get_literal
        con = sign*vs.get_literal(value)

        # on-the-fly simplify
        if ass.is_fixed(lit) and not ass.is_fixed(con):
            ret, con = self.update_literal(vs, value, cc, sign > 0)
            if not ret:
                return False
            con = sign*con

        # propagate the literal
        if not ass.is_true(con):
            if not cc.add_clause([-lit, con]):
                return False

        return True

    def _propagate_variables(self, cc, vs, reason_lit, consequences, sign):
        for value, lit in consequences:
            if cc.assignment.is_true(sign*lit):
                break
            if not self._propagate_variable(cc, vs, value, reason_lit, sign):
                return False
            # Note: Literals might be uppdated on level 0 and the reason_lit is
            # already guaranteed to be a fact on level 0.
            if self.config.propagate_chain and cc.assignment.decision_level > 0:
                reason_lit = sign*lit

        return True

    def _update_constraints(self, var, diff):
        """
        Traverses the lookup tables for constraints removing inactive
        constraints.

        The parameters determine whether the lookup tables for lower or upper
        bounds are used.
        """
        lvl = self._level

        l = self.var_watches_.get(var, [])
        i = 0
        for j, (co, cs) in enumerate(l):
            if not cs.removable(lvl.level):
                if cs.update(co, diff):
                    self._todo.add(cs)
                if i < j:
                    l[i], l[j] = l[j], l[i]
                i += 1
            else:
                lvl.removed_v2cs.append((var, co, cs))
        del l[i:]

    def _update_domain(self, cc, lit):
        """
        If `lit` is an order literal, this function updates the lower or upper
        bound associated to the variable of the literal (if necessary).
        Furthermore, the preceeding or succeeding order literal is propagated
        if it exists.
        """
        ass = cc.assignment
        assert ass.is_true(lit)

        lvl = self._level

        # update and propagate upper bound
        if lit in self._litmap:
            start = self._facts_integrated[0] if lit == TRUE_LIT else None
            for vs, value in self._litmap[lit][start:]:
                # update upper bound
                if vs.upper_bound > value:
                    diff = value - vs.upper_bound
                    if ass.decision_level > 0 and lvl.undo_upper.add(vs):
                        vs.push_upper()
                    vs.upper_bound = value
                    self._udiff.setdefault(vs.var, 0)
                    self._udiff[vs.var] += diff

                # make succeeding literals true
                if not self._propagate_variables(cc, vs, lit, vs.succ_values(value), 1):
                    return False

        # update and propagate lower bound
        if -lit in self._litmap:
            start = self._facts_integrated[1] if lit == TRUE_LIT else None
            for vs, value in self._litmap[-lit][start:]:
                # update lower bound
                if vs.lower_bound < value+1:
                    diff = value+1-vs.lower_bound
                    if ass.decision_level > 0 and lvl.undo_lower.add(vs):
                        vs.push_lower()
                    vs.lower_bound = value+1
                    self._ldiff.setdefault(vs.var, 0)
                    self._ldiff[vs.var] += diff

                # make preceeding literals false
                if not self._propagate_variables(cc, vs, lit, vs.prev_values(value), -1):
                    return False

        return True

    def mark_inactive(self, cs):
        """
        Mark the given constraint inactive on the current level.
        """
        lvl = self._level
        if cs.tagged_removable and not cs.marked_inactive:
            cs.marked_inactive = lvl.level
            lvl.inactive.append(cs)

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
            self._litmap.setdefault(lit, []).append((vs, value))

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

    @measure_time_decorator("statistics.time_undo")
    def undo(self):
        """
        This function undos decision level specific state.

        This includes undoing changed bounds of variables clearing constraints
        that where not propagated on the current decision level.
        """
        lvl = self._level

        for vs in lvl.undo_lower:
            value = vs.lower_bound
            vs.pop_lower()
            diff = value - vs.lower_bound - self._ldiff.get(vs.var, 0)
            if diff != 0:
                for co, cs in self.var_watches_.get(vs.var, []):
                    cs.undo(co, diff)
        self._ldiff.clear()

        for vs in lvl.undo_upper:
            value = vs.upper_bound
            vs.pop_upper()
            diff = value - vs.upper_bound - self._udiff.get(vs.var, 0)
            if diff != 0:
                for co, cs in self.var_watches_.get(vs.var, []):
                    cs.undo(co, diff)
        self._udiff.clear()

        for cs in lvl.inactive:
            cs.mark_active()

        for var, co, cs in lvl.removed_v2cs:
            self.var_watches_[var].append((co, cs))

        assert len(self._levels) > 1
        self._levels.pop()
        # Note: To make sure that the todo list is cleared when there is
        #       already a conflict during propagate.
        self._todo.clear()

    # checking
    @property
    def _num_facts(self):
        """
        The a pair of intergers corresponding to the numbers of order literals
        associated with the true and false literal.
        """
        t = len(self._litmap.get(TRUE_LIT, []))
        f = len(self._litmap.get(-TRUE_LIT, []))
        return t, f

    @measure_time_decorator("statistics.time_check")
    def check(self, cc, check_state):
        """
        This functions propagates facts that have not been integrated on the
        current level and propagates constraints gathered during `propagate`.
        """
        ass = cc.assignment
        lvl = self._level
        # Note: Most of the time check has to be called only for levels that
        # have also been propagated. The exception is if a minimize constraint
        # has to be integrated when backtracking from a bound update.
        if ass.decision_level != lvl.level and lvl.level >= self._minimize_level:
            return True

        # Note: We have to loop here because watches for the true/false
        # literals do not fire again.
        while True:
            # Note: This integrates any facts that have not been integrated yet
            # on the top level.
            if self._facts_integrated != self._num_facts:
                assert ass.decision_level == 0
                if not self._update_domain(cc, 1):
                    return False
                self._facts_integrated = self._num_facts

            # update the bounds of the constraints
            for var, diff in self._udiff.items():
                self._update_constraints(var, diff)
            self._udiff.clear()
            for var, diff in self._ldiff.items():
                self._update_constraints(var, diff)
            self._ldiff.clear()

            # propagate affected constraints
            todo, self._todo = self._todo, TodoList()
            for cs in todo:
                if not ass.is_false(cs.literal):
                    if not cs.propagate(self, cc, self.config, check_state):
                        return False
                else:
                    self.mark_inactive(cs)

            if self._facts_integrated == self._num_facts:
                return True

    def check_full(self, control, check_solution):
        """
        This function selects a variable that is not fully assigned w.r.t. the
        current assignment and introduces an additional order literal for it.

        This function should only be called total assignments.
        """
        post = range(self._lerp_last, len(self.var2vs_))
        pre = range(0, self._lerp_last)
        for i in chain(post, pre):
            vs = self.var2vs_[i]
            if not vs.is_assigned:
                self._lerp_last = i
                value = lerp(vs.lower_bound, vs.upper_bound)
                self.get_literal(vs, value, control)
                return

        if check_solution:
            for lit, constraints in self._l2c.items():
                if control.assignment.is_true(lit):
                    for c in constraints:
                        assert self.constraint_state(c).check_full(self)

    # reinitialization
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
        for lit, vss in self._litmap.items():
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
            del self._litmap[lit]

        # Note: Map bounds associated with top level facts to true/false.
        # Because we do not know if the facts have already been propagated, we
        # simply append them and do not touch the counts for integrated facts.
        for old, vss in sorted(remove_fixed):
            for vs, value in vss:
                lit = TRUE_LIT if ass.is_true(old) else -TRUE_LIT
                self._litmap.setdefault(lit, []).append((vs, value))
                vs.set_literal(value, lit)
            del self._litmap[old]

    def _cleanup_literals(self, cc, lit, pred):
        """
        Remove (var,value) pairs associated with `lit` that match `pred`.
        """
        assert lit in (TRUE_LIT, -TRUE_LIT)
        if lit in self._litmap:
            variables = self._litmap[lit]

            # adjust the number of facts that have been integrated
            idx = 0 if lit == TRUE_LIT else 1
            nums = list(self._facts_integrated)
            for x in variables[:nums[idx]]:
                if pred(x):
                    nums[idx] -= 1
            self._facts_integrated = tuple(nums)

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
        for vs_b, _ in other._litmap.get(TRUE_LIT, []):
            vs_a = self.var2vs_[vs_b.var]
            if vs_b.upper_bound < vs_a.upper_bound:
                ret, _ = self.update_literal(vs_a, vs_b.upper_bound, cc, True)
                if not ret:
                    return False

        # update lower bounds
        for vs_b, _ in other._litmap.get(-TRUE_LIT, []):
            vs_a = self.var2vs_[vs_b.var]
            if vs_a.lower_bound < vs_b.lower_bound:
                ret, _ = self.update_literal(vs_a, vs_b.lower_bound-1, cc, False)
                if not ret:
                    return False

        return self._update_domain(cc, 1)
*/

} // namespace Clingcon
